#include "main.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <memory>
#include <chrono>
#include <exception>
#include <unistd.h>
#include <tgbot/tgbot.h>
#include <ArduinoJson.h>
#include <pigpio.h>
#include <thread>
#include <atomic>
#include <csignal>
#include "mongoose.h"
#include <sqlite3.h>
#include "json.hpp"
#include <limits.h>
#include "sql.hpp"
#include "ssd1306.hpp"
#include "log.hpp"
#include "bmp180.hpp"
#include "bot_utils.hpp"
#include "TrendTracker.h"
#include "correctHumidity.hpp"
#include "SunriseSunset_io.hpp"
#include "moonPhases.hpp"
#include "charts.hpp"
#include "version.hpp"
#include "secret.h"

std::mutex sensorMutex;
std::atomic<bool> keepRunning(true);
double ds18b20_temp = 0;
double bmp180_temp = 0;
double bmp180_pressure = 0;
int dht11_temp = 0;
int dht11_humidity = 0;
int dht11_humidity_orig = 0;
bool dht11_ok = false;
Log gLog;
sqlite3* db;
unsigned int SQLRecordCounter = 0;

TrendTracker      trendTemperature;
TrendTracker      trendPressure;
TrendTracker      trendHumidity;

bool isSystemd = false;
float temperatureThreshold = TEMPERATURE_THRESHOLD;
float pressureThreshold    = PRESSURE_THRESHOLD;
float humidityThreshold    = HUMIDITY_THRESHOLD;
float humidityCorrection   = HUMIDITY_CORRECTION;
std::string  ownerChatId   = OWNER_CHAT_ID;
auto start_time = std::chrono::steady_clock::now();

bool loadConfig() {
	std::ifstream file(CFG_FILE);
	if (!file) {
		gLog.write("Error: Failed to open config file '%s'", CFG_FILE);
		return false;
	}
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    DynamicJsonDocument doc(512);
	DeserializationError error = deserializeJson(doc, file);
	file.close();
	if (error) {
		gLog.write("Error: JSON parse: ");
        gLog.write(error.c_str());
		return false;
	}
	if (doc.containsKey("ownerChatId")) ownerChatId = doc["ownerChatId"].as<std::string>();
	if (doc.containsKey("temperatureThreshold")) temperatureThreshold = doc["temperatureThreshold"].as<float>();
	if (doc.containsKey("humidityThreshold")) humidityThreshold = doc["humidityThreshold"].as<float>();
	if (doc.containsKey("pressureThreshold")) pressureThreshold = doc["pressureThreshold"].as<float>();
    if (doc.containsKey("humidityCorrection")) humidityCorrection = doc["humidityCorrection"].as<float>();

	gLog.write("Config loaded from '%s'", CFG_FILE);
	return true;
}
bool saveConfig() {
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    DynamicJsonDocument doc(512);

	doc["ownerChatId"]          = ownerChatId;
	doc["temperatureThreshold"] = temperatureThreshold;
	doc["pressureThreshold"]    = pressureThreshold;
	doc["humidityThreshold"]    = humidityThreshold;
    doc["humidityCorrection"]   = humidityCorrection;

	std::ofstream file(CFG_FILE);

	if (!file) {
		gLog.write("Error: failed to open config file '%s' for writing", CFG_FILE);
		return false;
	}

	if (serializeJsonPretty(doc, file) == 0) {
		gLog.write("Error: failed to write JSON");
		file.close();
		return false;
	}

	file.close();
	gLog.write("Config file '%s' saved", CFG_FILE);
	return true;
}
std::string APISaveConfig(struct mg_http_message *hm) {
    char buf[128];

    if (mg_http_get_var(&hm->query, "ownerChatId", buf, sizeof(buf)) <= 0) {
        gLog.write("Error: failed option 'ownerChatId'");
        return "{\"status\":\"ERROR\"}";
    }
    ownerChatId = std::string(buf);

    if (mg_http_get_var(&hm->query, "temperatureThreshold", buf, sizeof(buf)) <= 0) {
        gLog.write("Error: failed option 'temperatureThreshold'");
        return "{\"status\":\"ERROR\"}";
    }
    temperatureThreshold = atof(buf);
    trendTemperature.threshold = temperatureThreshold;


    if (mg_http_get_var(&hm->query, "humidityThreshold", buf, sizeof(buf)) <= 0) {
        gLog.write("Error: failed option 'humidityThreshold'");
        return "{\"status\":\"ERROR\"}";
    }
    humidityThreshold = atof(buf);
    trendHumidity.threshold = humidityThreshold;

    if (mg_http_get_var(&hm->query, "pressureThreshold", buf, sizeof(buf)) <= 0) {
        gLog.write("Error: failed option 'pressureThreshold'");
        return "{\"status\":\"ERROR\"}";
    }
    pressureThreshold = atof(buf);
    trendPressure.threshold = pressureThreshold;

    if (mg_http_get_var(&hm->query, "humidityCorrection", buf, sizeof(buf)) <= 0) {
        gLog.write("Error: failed option 'humidityCorrection'");
        return "{\"status\":\"ERROR\"}";
    }
    humidityCorrection = atof(buf);

    if (saveConfig()) {
        return "{\"status\":\"OK\"}";
    } else {
        return "{\"status\":\"ERROR\"}";
    }
}
int get_executable_path(char *buf, size_t size) {
    if (!buf || size == 0) return -1;
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len == -1) {
        return -1;
    }
    buf[len] = '\0';
    return 0;
}
auto uptime() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
}
std::string printUptime() {
    auto seconds = uptime();

    int days = seconds / 86400;
    int hrs  = (seconds % 86400) / 3600;
    int mins = (seconds % 3600) / 60;
    int secs = seconds % 60;

    std::ostringstream ret;
    if (days > 0) {
        ret << days << " day(s) "
            << hrs << ":"
            << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    } else {
        ret << hrs << ":"
            << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    }

    return ret.str();
}
void startSensorThread() {
    std::thread([] {
        BMP180 bmp;
        static bool sentLog = false;
        static bool firstTime = true;
        if (!bmp.begin()) {
            std::cerr << "BMP180 init failed in sensor thread\n";
            return;
        }

        while (true) {
            double t_ds  = readDS18B20();
            double t_bmp = bmp.readTemperature();
            double p_bmp = bmp.readPressure();
            int t_dht = 0, h_dht = 0;
            bool ok_dht = false;

            int retries = 10;
            while (!ok_dht && retries--) {
                ok_dht = readDHT11(DHT11_GPIO, t_dht, h_dht); // GPIO 22 Pin 15
                usleep(40000);
            }

            {
                std::lock_guard<std::mutex> lock(sensorMutex);
                ds18b20_temp = t_ds;
                TrendTracker_add(&trendTemperature, static_cast<float>(ds18b20_temp));
                bmp180_temp = t_bmp;
                bmp180_pressure = p_bmp;
                TrendTracker_add(&trendPressure, static_cast<float>(bmp180_pressure));
                if(ok_dht) {
                    dht11_humidity = correctHumidity(h_dht, t_dht, ds18b20_temp, humidityCorrection);
                    TrendTracker_add(&trendHumidity, static_cast<float>(dht11_humidity));
                } else {
                    TrendTracker_add(&trendHumidity, static_cast<float>(dht11_humidity));
                    gLog.write("Error reading DHT11");
                }
                dht11_humidity_orig = h_dht;
                dht11_temp = t_dht;
                dht11_ok = ok_dht;
                insertRecord(db, 
                    static_cast<float>(ds18b20_temp),
                    static_cast<float>(bmp180_pressure),
                    static_cast<float>(dht11_humidity)
                );
                SQLRecordCounter ++;
                if(SQLRecordCounter > 10) {
                    SQLRecordCounter = 0;
                    trimOldRecords(db);
                }
                //auto seconds = uptime();
                time_t seconds = time(nullptr); 
                if(((seconds % 3600) < 60 && !sentLog) || firstTime) {
                    gLog.write("updateAutoBrightness: %d", updateAutoBrightness());
                    std::ostringstream toLog;
                    toLog << std::fixed << std::setprecision(2);
                    toLog << "ds18b20_temp:" << ds18b20_temp << "°C, ";
                    toLog << "bmp180_pressure:" << (bmp180_pressure  * 0.75006375541921 / 100.) << "mmHg, ";
                    toLog << "dht11_humidity:" << dht11_humidity << "%%";
                    gLog.write(toLog.str().c_str());
                    //toLog.str("");
                    //toLog.clear();
                    //std::string moonUrl = buildMoonPhaseURL();
                    //toLog << "Start loading: " << moonUrl;
                    //gLog.write(toLog.str().c_str());
                    //if(downloadMoonPhaseJSON(moonUrl, MOONPHASES_FILE)) {
                    //    gLog.write("MoonPhases: [" MOONPHASES_FILE "] Ok!");
                    //} else {
                    //    gLog.write("MoonPhases: [" MOONPHASES_FILE "] Error!");
                    //}
                    if((seconds % 43200) < 60 || firstTime) {
                        std::string sunUrl = generateSunAPIUrl(LATITUDE, LONGITUDE);
                        toLog.str("");
                        toLog.clear();
                        toLog << "Start loading: " << sunUrl;
                        gLog.write(toLog.str().c_str());

                        if(downloadSunData(sunUrl, SUNRISESUNSET_FILE)) {
                            gLog.write("SunriseSunset: [" SUNRISESUNSET_FILE "] Ok!");
                        } else {
                            gLog.write("SunriseSunset: [" SUNRISESUNSET_FILE "] Error!");
                        }
                    }
                    sentLog = true;
                    firstTime = false;
                } else if((seconds % 3600) >= 60 && sentLog) {
                    sentLog = false;
                }
                makeChart(CHARTS_PNG_FILE, exportToJson(db), gLog);
            }

            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }).detach();
}
bool isRainLikely() {
    float pressureSlope = TrendTracker_getSlope(&trendPressure);
    float tempSlope = TrendTracker_getSlope(&trendTemperature);

    if (
        (pressureSlope < -0.03f && dht11_humidity > 65) || 
        (pressureSlope < -0.01f && dht11_humidity > 60 && tempSlope < 0.0f) ||
        (pressureSlope <= 0.0f && dht11_humidity > 90)
    ) return true;
    return false;
}
std::ostringstream __meteoMsg(std::string user) {
    std::string isRain = RAIN_LIKELY;
    if (!isRainLikely()) {
        isRain =  NO_RAIN_EXP;
    }
    nlohmann::json todayData;
    if(!getTodaySunData(SUNRISESUNSET_FILE, todayData)) {
        gLog.write("Error getting todayData from %s", SUNRISESUNSET_FILE);
    }
//    nlohmann::json todayMoon;
//    if(!getTodayMoonPhase(MOONPHASES_FILE, todayMoon)) {
//        gLog.write("Error getting todayMoon from %s", MOONPHASES_FILE);
//    }

    MeteoData mData = {
        .user = user,
        .ds18b20_temp = ds18b20_temp,
        .bmp180_pressure = bmp180_pressure,
        .dht11_humidity = dht11_humidity,
        .dht11_ok = dht11_ok,
        .arrowT = TrendTracker_getArrow(&trendTemperature),
        .arrowP = TrendTracker_getArrow(&trendPressure),
        .arrowH = TrendTracker_getArrow(&trendHumidity),
        .isRainLikely = isRain,
        .sunrise = todayData["sunrise"],
        .sunset = todayData["sunset"],
        .day_length = todayData["day_length"],
        .solar_noon = todayData["solar_noon"]//,
//        .moonPhase = todayMoon.empty() ? "no moon data" : formatMoonPhaseInfo(todayMoon)
    };
    return(meteoMsg(mData));
}
void setMenu(TgBot::Bot& bot) {
    std::vector<TgBot::BotCommand::Ptr> commands;

    TgBot::BotCommand::Ptr cmd1(new TgBot::BotCommand());
    cmd1->command = "start";
    cmd1->description = "Start the bot";
    commands.push_back(cmd1);

    TgBot::BotCommand::Ptr cmd2(new TgBot::BotCommand());
    cmd2->command = "meteo";
    cmd2->description = "Show current weather";
    commands.push_back(cmd2);

    TgBot::BotCommand::Ptr cmd3(new TgBot::BotCommand());
    cmd3->command = "status";
    cmd3->description = "System info";
    commands.push_back(cmd3);

    bot.getApi().setMyCommands(commands);
}
void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str(API_TEMP_DATA), NULL)) {
            std::string json = exportToJson(db);
            mg_http_reply(c, 200, "Content-Type: application/json\r\nCache-Control: no-cache\r\n", "%s", json.c_str());
        } else if (mg_match(hm->uri, mg_str(API_LOG), NULL)) {
            struct mg_http_serve_opts opts = {
                .extra_headers =
                    "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
                    "Pragma: no-cache\r\n"
                    "Expires: 0\r\n"
                    "Content-Type: text/plain\r\n"
            };
            mg_http_serve_file(c, hm, gLog.getLogFilePath(), &opts);
        } else if (mg_match(hm->uri, mg_str(API_DASHBOARD), NULL)) {
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            DynamicJsonDocument doc(512);
            std::string isRain = RAIN_LIKELY_SHRT;
            if (!isRainLikely()) {
                isRain =  NO_RAIN_EXP_SHRT;
            }
            char tempStr[16];
            snprintf(tempStr, sizeof(tempStr), "%+0.2f", ds18b20_temp);
            doc["temperature"]    = tempStr;
            doc["pressure"]       = bmp180_pressure;
            doc["humidity"]       = dht11_humidity;
            doc["humidity_orig"]  = dht11_humidity_orig;
            doc["bmp180_temp"]    = bmp180_temp;
            doc["dht11_temp"]     = dht11_temp;
            doc["temperature_a"]  = TrendTracker_getArrow(&trendTemperature);
            doc["pressure_a"]     = TrendTracker_getArrow(&trendPressure);
            doc["humidity_a"]     = TrendTracker_getArrow(&trendHumidity);
            doc["isRain"]         = isRain;
            doc["version"]        = getVersionFull();
            auto cpuHzStr         = execCmd("vcgencmd measure_clock arm | cut -d'=' -f2");
            auto gpuHzStr         = execCmd("vcgencmd measure_clock core | cut -d'=' -f2");
            doc["cpuMHz"]         = cpuHzStr.empty() ? 0 : std::stod(cpuHzStr) / 1e6;
            doc["gpuMHz"]         = gpuHzStr.empty() ? 0 : std::stod(gpuHzStr) / 1e6;
            doc["cpuTemp"]        = readCpuTemp();
            doc["fan"]            = (shmMemRead() ? "On" : "Off");
            std::string throttled = execCmd("vcgencmd get_throttled | cut -d'=' -f2");
            doc["throttled"]      = decodeThrottledFlags(throttled);
            doc["isSystemd"]      = (isSystemd ? "systemd" : "manual");
            doc["ssid"]           = execCmd("iwgetid -r");
            doc["rssi"]           = std::stoi(execCmd("iw dev wlan0 link | awk '/signal/ {print $(NF-1) $NF}'"));
            doc["uptime"]         = printUptime();
            nlohmann::json sunSet;
            if(getTodaySunData(SUNRISESUNSET_FILE, sunSet)) {
                std::string jsonStr = sunSet.dump();
                DynamicJsonDocument nestedDoc(128);
                deserializeJson(nestedDoc, jsonStr);
                doc["sunSet"] = nestedDoc.as<JsonObject>();
            }
            nlohmann::json todayMoon;
            if(getTodayMoonPhase(MOONPHASES_FILE, todayMoon)) {
                std::string jsonStr = todayMoon.dump();
                DynamicJsonDocument nestedDoc(128);
                deserializeJson(nestedDoc, jsonStr);
                doc["todayMoon"] = nestedDoc.as<JsonObject>();
            }
            std::string json;
            serializeJsonPretty(doc, json);
            mg_http_reply(c, 200, "Content-Type: application/json\r\nCache-Control: no-cache\r\n", "%s", json.c_str());
        } else if (mg_match(hm->uri, mg_str(API_CONF_SAVE), NULL)) {
            std::string json = APISaveConfig(hm);
            mg_http_reply(c, 200, "Content-Type: application/json\r\nCache-Control: no-cache\r\n", "%s", json.c_str());
        } else if (mg_match(hm->uri, mg_str(API_CONF_DATA), NULL)) {
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            DynamicJsonDocument doc(512);
            doc["ownerChatId"]          = ownerChatId;
            doc["temperatureThreshold"] = temperatureThreshold;
            doc["pressureThreshold"]    = pressureThreshold;
            doc["humidityThreshold"]    = humidityThreshold;
            doc["humidityCorrection"]   = humidityCorrection;
            std::string json;
            serializeJsonPretty(doc, json);
            mg_http_reply(c, 200, "Content-Type: application/json\r\nCache-Control: no-cache\r\n", "%s", json.c_str());
        } else {
            struct mg_http_serve_opts opts = {
                .root_dir = WWW_PATH,
                .extra_headers =
                    "Cache-Control: public, max-age=86400\r\n"
                    "Expires: Thu, 31 Dec 2099 23:59:59 GMT\r\n"
            };
            mg_http_serve_dir(c, hm, &opts);
        }
    }
}
void runBot(TgBot::Bot& bot) {
    static int counter = 10;
    while(counter > 0) {
        try {
            TgBot::TgLongPoll longPoll(bot);
            while (keepRunning) {
                longPoll.start();
            }
        } catch (const std::exception& e) {
            gLog.write("TgBot error...");
            gLog.write(e.what());
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
        gLog.write("TgBot restarting longPoll %d", counter);
        counter --;
    }
}
void runWebServer() {

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_log_set(MG_LL_NONE);
    mg_http_listen(&mgr, "http://0.0.0.0:80", fn, &mgr);

    while (keepRunning) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
}
void signalHandler(int) {
    gLog.write("Daemon killed...");
    addMsg("Daemon killed...");
    keepRunning = false;
}
void killOtherInstances(const char *app_name, int timeout_ms) {
    char buffer[512];
    pid_t my_pid = getpid();

    char command[128];
    snprintf(command, sizeof(command), "pidof %s", app_name);

    FILE *pipe = popen(command, "r");
    if (!pipe || !fgets(buffer, sizeof(buffer), pipe)) {
        if (pipe) pclose(pipe);
        fprintf(stderr, "Failed to get PID list for %s\n", app_name);
        return;
    }
    pclose(pipe);

    char *token = strtok(buffer, " \n");
    while (token) {
        pid_t pid = strtoul(token, NULL, 10);
        if (pid > 0 && pid != my_pid) {
            printf("Old PID found, kill \e[31m%d\e[0m ... ", pid);
            if (kill(pid, SIGTERM) == 0) {
                printf("\e[33m[term]\e[0m ");
                usleep(timeout_ms * 1000);
                if (kill(pid, 0) == 0) {
                    if (kill(pid, SIGKILL) == 0) {
                        printf("\e[31m[kill]\e[0m\n");
                    } else {
                        printf("\e[31m[kill error]\e[0m\n");
                    }
                } else {
                    printf("\e[32m[done]\e[0m\n");
                }
            } else {
                printf("\e[31m[term error]\e[0m\n");
            }
        }
        token = strtok(NULL, " \n");
    }
}
int main(int argc, char* argv[]) {

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--systemd") {
            isSystemd = true;
            break;
        }
    }

    if(!isSystemd) killOtherInstances(APP_NAME, 1000);
    gLog.setLogPath(LOG_PATH);
    gLog.write("%s started", APP_NAME);

    char myPath[PATH_MAX];
    if (get_executable_path(myPath, sizeof(myPath)) == 0) {
        gLog.write("Executable path: %s\n", myPath);
    } else {
        gLog.write("Error: get_executable_path");
    }

    pid_t myPID = getpid();

    if (sqlite3_open(SQLITE_DB_FILE, &db) != SQLITE_OK) {
        gLog.write("Failed to open DB %s", SQLITE_DB_FILE);
        std::cerr << "Failed to open DB " << SQLITE_DB_FILE << "/n";
        return 1;
    }

    initDb(db, gLog);

    if (gpioInitialise() < 0) {
        gLog.write("pigpio init failed");
        std::cerr << "pigpio init failed\n";
        return 1;
    }

    if(!loadConfig()) saveConfig();

    initDisplay();

    addMsg(APP_NAME " started");
    if(isSystemd) {
        addMsg("SYSTEMD");
    } else {
        addMsg("MANUAL");
    }

    TrendTracker_init(&trendTemperature, temperatureThreshold);
    TrendTracker_init(&trendPressure, pressureThreshold);
    TrendTracker_init(&trendHumidity, humidityThreshold);

    TgBot::Bot bot(TOKEN);

    setMenu(bot);

    using namespace TgBot;

    TgBot::ReplyKeyboardMarkup::Ptr keyboard(new TgBot::ReplyKeyboardMarkup);
    keyboard->resizeKeyboard = true;
    keyboard->oneTimeKeyboard = false;

    std::vector<TgBot::KeyboardButton::Ptr> row;
    TgBot::KeyboardButton::Ptr btn0(new TgBot::KeyboardButton);
    btn0->text = "🌡 Weather";
    btn0->requestContact = false;
    btn0->requestLocation = false;
    row.push_back(btn0);
    TgBot::KeyboardButton::Ptr btn1(new TgBot::KeyboardButton);
    btn1->text = "📈 Charts";
    btn1->requestContact = false;
    btn1->requestLocation = false;
    row.push_back(btn1);
    keyboard->keyboard.push_back(row);
    std::vector<TgBot::KeyboardButton::Ptr> row1;
    TgBot::KeyboardButton::Ptr btn2(new TgBot::KeyboardButton);
    btn2->text = "💻 Status";
    btn2->requestContact = false;
    btn2->requestLocation = false;
    row1.push_back(btn2);
    keyboard->keyboard.push_back(row1);

    bot.getEvents().onAnyMessage([&](Message::Ptr message) {
        if (message->text == "/start") {
            TgBot::ReplyKeyboardRemove::Ptr removeKeyboard(new TgBot::ReplyKeyboardRemove);
            removeKeyboard->removeKeyboard = true;
            bot.getApi().sendMessage(
                message->chat->id,
                "👋 Hello, <b>" + message->from->username + "</b>!",
                nullptr,
                nullptr,
                removeKeyboard,
                "HTML"
            );
            bot.getApi().sendMessage(
                message->chat->id,
                "Use '/meteo' or one of the options below to see weather and status info",
                nullptr,
                nullptr,
                keyboard,
                "HTML"
            );
            if (message && message->from) {
                std::string toLog = "Reply to message '/start' from @" + 
                    message->from->username + 
                    ", chatID=" + std::to_string(message->chat->id);
                gLog.write(toLog.c_str());
                std::string cmsg = "'/start': @" + message->from->username;
                addMsg(cmsg.c_str());

            }
        } else if (message->text == "💻 Status" || message->text == "/status") {
            std::string sys = buildSystemStatus();
            bot.getApi().sendMessage(message->chat->id, sys, nullptr, nullptr, nullptr, "HTML");
            if (message && message->from) {
                std::string toLog = "Reply to message '/status' from @" + 
                    message->from->username + 
                    ", chatID=" + std::to_string(message->chat->id);
                gLog.write(toLog.c_str());
                std::string cmsg = "'/status': @" + message->from->username;
                addMsg(cmsg.c_str());
            }
        } else if (message->text == "🌡 Weather" || message->text == "/meteo") {
            std::ostringstream sens = __meteoMsg(message->from->username);
            bot.getApi().sendMessage(message->chat->id, sens.str(), nullptr, nullptr, nullptr, "HTML");
            if (message && message->from) {
                std::string toLog = "Reply to message '/meteo' from '@" + 
                    message->from->username +
                    ", chatID=" + std::to_string(message->chat->id);
                gLog.write(toLog.c_str());
                std::string cmsg = "'/meteo': @" + message->from->username;
                addMsg(cmsg.c_str());
            }
        } else if (message->text == "/charts" ||message->text == "📈 Charts" ) {
            std::ifstream ifs(CHARTS_PNG_FILE, std::ios::binary);
            if (!ifs) {
                gLog.write("Can't open file: %s", CHARTS_PNG_FILE);
                //addMsg("ERROR!");
                return;
            }

            std::stringstream buffer;
            buffer << ifs.rdbuf();
            std::string fileData = buffer.str();

            TgBot::InputFile::Ptr inputFile(new TgBot::InputFile);
            inputFile->data = fileData;
            inputFile->fileName = CHART_PNG_NAME;

            bot.getApi().sendPhoto(message->chat->id, inputFile, "");
            
            if (message && message->from) {
                std::string toLog = "Reply to message '/chart' from '@" + 
                    message->from->username +
                    ", chatID=" + std::to_string(message->chat->id);
                gLog.write(toLog.c_str());
                std::string cmsg = "'/charts': @" + message->from->username;
                addMsg(cmsg.c_str());
            }
        } else {
            if (message && message->from) {
                std::string toLog = "Unknown message '" + message->text + "' from '@" + 
                    message->from->username +
                    ", chatID=" + std::to_string(message->chat->id);
                gLog.write(toLog.c_str());
                std::string cmsg = "Unknown message";
                addMsg(cmsg.c_str());
            }
        }

    });

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "\e[31mError: Start Daemon failed\e[0m\n";
        gLog.write("Error: Start Daemon failed\n");
        return -1;
    } else if (pid == 0) {
        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        addMsg("Daemon started...");
        addMsg(getVersionFull());
        gLog.write("Daemon started V%s", getVersionFull());
        gLog.write("PID: %d", getpid());
        startSensorThread();
        addMsg("Start 'SensorThread'");
        gLog.write("Start 'SensorThread'");
        TgBot::User::Ptr me = bot.getApi().getMe();
        gLog.write("Start telegram-bot '@%s'", me->username.c_str());
        addMsg(me->username.c_str());
        std::thread telegramThread(runBot, std::ref(bot));
        gLog.write("Start WEB-server");
        addMsg("Start WEB-server");
        runWebServer();
        telegramThread.join();
        gpioTerminate();
        gLog.write("Exit...");
    }
    gpioTerminate();
    char PIDSbuf[512];
    FILE *cmd_pipe_new = popen("pidof " APP_NAME, "r");
    if (cmd_pipe_new && fgets(PIDSbuf, sizeof(PIDSbuf), cmd_pipe_new)) {
        pclose(cmd_pipe_new);

        char *token = strtok(PIDSbuf, " \n");
        while (token) {
            pid_t pid = strtoul(token, NULL, 10);
            if (pid != myPID) {
              printf("New PID \e[32m%d\e[0m\n", pid);
            }
            token = strtok(NULL, " \n");
        }
    } else {
        if (cmd_pipe_new) pclose(cmd_pipe_new);
        fprintf(stderr, "Failed to read PID list\n");
    }
    return 0;
}

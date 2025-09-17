#include <array>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath> 
#include <cstdint>
#include <sys/stat.h>
#include <ctime>
#include <sys/mman.h>
// --- POSIX / Linux
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <pigpio.h>
#include "main.hpp"
#include "version.hpp"
#include "bot_utils.hpp"


std::string execCmd(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    // trim
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

/************************************
 * CPU temperature helper          *
 ***********************************/
double readCpuTemp() {
    FILE* f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return NAN;
    int milli;
    fscanf(f, "%d", &milli);
    fclose(f);
    return milli / 1000.0;
}

/************************************
 * Sensor: DS18B20 via sysfs       *
 ***********************************/
double readDS18B20() {
    static std::string devPath;
    if (devPath.empty()) {
        DIR* dir = opendir("/sys/bus/w1/devices");
        if (!dir) return NAN;
        dirent* de;
        while ((de = readdir(dir)))
            if (!strncmp(de->d_name, "28-", 3))
                devPath = std::string("/sys/bus/w1/devices/") + de->d_name + "/w1_slave";
        closedir(dir);
        if (devPath.empty()) return NAN;
    }
    std::ifstream fs(devPath);
    std::string line;
    std::getline(fs, line);
    std::getline(fs, line);
    auto pos = line.find("t=");
    if (pos == std::string::npos) return NAN;
    int milli = std::stoi(line.substr(pos + 2));
    return milli / 1000.0;
}

/************************************
 * Sensor: DHT11 via pigpio        *
 ***********************************/
bool readDHT11(int gpioPin, int& temp_c, int& hum) {
    uint8_t bits[5] = {0};
    gpioSetMode(gpioPin, PI_OUTPUT);
    gpioWrite(gpioPin, PI_LOW);
    gpioDelay(20000);
    gpioWrite(gpioPin, PI_HIGH);
    gpioDelay(40);
    gpioSetMode(gpioPin, PI_INPUT);

    int cnt = 0;
    while (gpioRead(gpioPin) == PI_HIGH) {
        if (++cnt > 100) return false;
        gpioDelay(1);
    }
    cnt = 0;
    while (gpioRead(gpioPin) == PI_LOW) {
        if (++cnt > 100) return false;
        gpioDelay(1);
    }
    cnt = 0;
    while (gpioRead(gpioPin) == PI_HIGH) {
        if (++cnt > 100) return false;
        gpioDelay(1);
    }

    for (int i = 0; i < 40; ++i) {
        cnt = 0;
        while (gpioRead(gpioPin) == PI_LOW) {
            if (++cnt > 100) return false;
            gpioDelay(1);
        }

        uint32_t t = 0;
        while (gpioRead(gpioPin) == PI_HIGH) {
            if (++t > 100) return false;
            gpioDelay(1);
        }

        bits[i / 8] <<= 1;
        if (t > 30) bits[i / 8] |= 1;
    }

    if (((bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF) != bits[4]) return false;
    hum    = bits[0];
    temp_c = bits[2];
    return true;
}
std::string getWifiIcon(int rssi) {
    if (rssi >= -50) return u8" ▇▇▇";
    else if (rssi >= -60) return u8" ▇▇▂";
    else if (rssi >= -70) return u8" ▇▂▁";
    else if (rssi >= -80) return u8" ▂▁ ";
    else return u8" ▁  ";
}
std::string buildSystemStatus() {
    std::ostringstream ss;
    std::string ssid = execCmd("iwgetid -r");
    std::string ip   = execCmd("hostname -I | awk '{print $1}'");
    std::string signal = execCmd("iw dev wlan0 link | awk '/signal/ {print $(NF-1) $NF}'");

    ss << "💻 <b>System status:</b>\n\n";
    ss << "📶 <b>SSID:</b> " << (ssid.empty() ? "N/A" : ssid);
    if (!signal.empty()) {
        ss << "<code>" << getWifiIcon(std::stoi(signal)) << "</code>\n";
        ss << "🛰️ <b>RSSI:</b> " << signal << "\n";
    }
    ss << "🌐 <b>IP:</b> " << ip << "\n";

    // Frequencies
    auto cpuHzStr  = execCmd("vcgencmd measure_clock arm | cut -d'=' -f2");
    auto gpuHzStr  = execCmd("vcgencmd measure_clock core | cut -d'=' -f2");
    double cpuMHz = cpuHzStr.empty() ? 0 : std::stod(cpuHzStr) / 1e6;
    double gpuMHz = gpuHzStr.empty() ? 0 : std::stod(gpuHzStr) / 1e6;
    ss << std::fixed << std::setprecision(0);
    ss << "⚙️ <b>CPU:</b> " << cpuMHz << " MHz\n";
    ss << "🎞 <b>GPU:</b> " << gpuMHz << " MHz\n";

    // CPU temp
    double cpuT = readCpuTemp();
    if (!std::isnan(cpuT)) ss << "🌡 <b>CPU temp:</b> " << std::setprecision(1) << cpuT << " °C\n";
    if(shmMemRead()) {
        ss << u8"🌀 <b>Fan:</b> On\n";
    } else {
        ss << u8"🌀 <b>Fan:</b> Off\n";
    }
    // Throttling flags
    std::string throttled = execCmd("vcgencmd get_throttled | cut -d'=' -f2");
    ss << decodeThrottledFlags(throttled) << "\n";

    ss << "🛠️ <b>Version:</b> " << getVersionFull(); //VERSION << "." << BUILD;
    return ss.str();
}
std::string decodeThrottledFlags(const std::string& hexStr) {
    std::ostringstream out;
    unsigned int value = 0;

    try {
        value = std::stoul(hexStr, nullptr, 16);
    } catch (...) {
        return "⚠️ throttled: error";
    }

    if (value == 0) {
        out << "✔️ No throttling";
        return out.str();
    }

    if (value & (1 << 0))  out << "❗️ Under-voltage condition currently active";
    if (value & (1 << 1))  out << "❗️ CPU frequency is currently capped due to under-voltage";
    if (value & (1 << 2))  out << "❗️ CPU frequency is currently capped due to high temperature";
    if (value & (1 << 3))  out << "❗️ CPU is currently throttled due to high temperature";

    if (value & (1 << 16)) out << "⚠️ Under-voltage condition has occurred";
    if (value & (1 << 17)) out << "⚠️ CPU frequency was previously capped due to under-voltage";
    if (value & (1 << 18)) out << "⚠️ CPU frequency was previously capped due to high temperature";
    if (value & (1 << 19)) out << "⚠️ CPU was previously throttled due to high temperature";

    return out.str();
}
std::ostringstream meteoMsg(MeteoData& mData) {
    std::ostringstream sens;
    sens << std::fixed << std::setprecision(2);
    sens << "👋 Hello, <b>" << mData.user <<"</b>!\n" << "<b>──────────────────</b>\n";
    sens << "   🌡 <b>Temperature: " << mData.arrowT << "</b> <code>";
    sens << (mData.ds18b20_temp > 0 ? "+" : "") << mData.ds18b20_temp << " °C</code>\n";
    sens << "   🫧 <b>Pressure: " << mData.arrowP << "</b> <code>";
    sens << (mData.bmp180_pressure * 0.75006375541921 / 100.) << " mmHg (" << std::setprecision(3); 
    sens <<  (mData.bmp180_pressure/1000.) << " kPa)</code>\n";
    if (mData.dht11_ok) {
        sens << "   💧 <b>Humidity: " << mData.arrowH << "</b> <code>" << mData.dht11_humidity << "%</code>\n";
    } else {
        sens << "   💧 <b>Humidity:</b> <code>Error!</code>\n";
    }
    char dateStr[16]; time_t t = time(nullptr);
    std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", std::localtime(&t));
    sens << mData.isRainLikely << "\n<b>──────────────────</b>\n";
    sens << "<b>☀️ Sun Times for " << dateStr << ": </b>\n";
    sens << "   🌄 <b>Sunrise:</b> <code>" << mData.sunrise << "</code>\n";
    sens << "   🌇 <b>Sunset:</b> <code>" << mData.sunset << "</code>\n";
    sens << "   🌞 <b>Solar noon:</b> <code>" << mData.solar_noon << "</code>\n";
    sens << "   🕒 <b>Day length:</b> <code>" << mData.day_length << "</code>\n";
//    sens << mData.moonPhase;
    return(sens);
}
bool shmMemRead() {
    int shm = shm_open(FAN_OBJECT_NAME, O_RDONLY, 0777);
    if (shm == -1) return false;

    int fanmode;
    long unsigned int temperature;
    int pwmRange;
    unsigned long unixTime;
    pid_t fanPID;
    char fanVersion[6];

    void* map = mmap(nullptr, SHARED_MEMORY_OBJECT_SIZE, PROT_READ, MAP_SHARED, shm, 0);
    if (map == MAP_FAILED) {
        close(shm);
        return false;
    }

    const char* msg = static_cast<const char*>(map);
    sscanf(msg, MEMORY_OBJECT_TEMPLATE, &fanmode, &temperature, &pwmRange, &unixTime, &fanPID, fanVersion);
    munmap(map, SHARED_MEMORY_OBJECT_SIZE);
    close(shm);
    if(fanmode) return true;
    return false;
}

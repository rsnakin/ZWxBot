#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "moonPhases.hpp"

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::vector<time_t> generateTimestamps(int days) {
    std::vector<time_t> timestamps;
    time_t now = time(nullptr);
    for (int i = 0; i < days; ++i) {
        timestamps.push_back(now + i * 86400); // 86400 секунд = 1 день
    }
    return timestamps;
}

std::string buildMoonPhaseURL() {
    std::vector<time_t> timestamps = generateTimestamps(30);
    std::string url = "https://api.farmsense.net/v1/moonphases/?";
    for (size_t i = 0; i < timestamps.size(); ++i) {
        url += "d[]=" + std::to_string(timestamps[i]);
        if (i != timestamps.size() - 1) url += "&";
    }
    return url;
}

bool downloadMoonPhaseJSON(const std::string& url, const std::string& filename) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return false;

    std::ofstream out(filename);
    if (!out.is_open()) return false;
    out << response;
    return true;
}

bool getTodayMoonPhase(const std::string& filename, json& todayEntry) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    json moonData;
    try {
        file >> moonData;
    } catch (...) {
        return false;
    }

    time_t now = time(nullptr);
    std::tm* localTime = std::localtime(&now);
    char dateStr[16];
    std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", localTime);

    for (const auto& entry : moonData) {
        if (entry.contains("TargetDate")) {
            time_t entryTime = 0;
            if (entry["TargetDate"].is_string()) {
                entryTime = std::stol(entry["TargetDate"].get<std::string>());
            } else if (entry["TargetDate"].is_number()) {
                entryTime = entry["TargetDate"].get<time_t>();
            } else {
                continue;
            }
            std::tm* entryTm = std::localtime(&entryTime);
            char entryStr[16];
            std::strftime(entryStr, sizeof(entryStr), "%Y-%m-%d", entryTm);
            if (std::string(entryStr) == dateStr) {
                todayEntry = entry;
                return true;
            }
        }
    }
    return false;
}

std::string getMoonPhaseIcon(const std::string& phase) {
    if (phase == "New Moon" || phase == "Dark Moon") return "🌑";
    if (phase == "Waxing Crescent") return "🌒";
    if (phase == "First Quarter") return "🌓";
    if (phase == "Waxing Gibbous") return "🌔";
    if (phase == "Full Moon") return "🌕";
    if (phase == "Waning Gibbous") return "🌖";
    if (phase == "Last Quarter") return "🌗";
    if (phase == "Waning Crescent") return "🌘";
    return "🌙";
}

std::string formatMoonPhaseInfo(const json& moon) {
    if (!moon.contains("Phase") || !moon.contains("Illumination") ||
        !moon.contains("Age") || !moon.contains("TargetDate")) {
        return "⚠️ Invalid moon data";
    }

    std::ostringstream out;
    std::string phase = moon["Phase"];
    std::string icon = getMoonPhaseIcon(phase);

    time_t t = std::stol(moon["TargetDate"].get<std::string>());
    char dateStr[16];
    std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", std::localtime(&t));

    out << "<b>──────────────────</b>\n🌙 <b>Moon phase for " << dateStr << ":</b>\n";
    out << "   " << icon << " <b>Phase:</b> <code>" << phase << "</code>\n";

    if (moon.contains("Moon") && moon["Moon"].is_array() && !moon["Moon"].empty()) {
        out << "   🏷️ <b>Moon name:</b> <code>" << moon["Moon"][0].get<std::string>() << "</code>\n";
    }

    out << "   🔆 <b>Illumination:</b> <code>"
        << std::fixed << std::setprecision(1)
        << moon["Illumination"].get<double>() * 100.0 << "%</code>\n";

    out << "   ⏳ <b>Moon age:</b> <code>"
        << std::fixed << std::setprecision(1)
        << moon["Age"].get<double>() << " days</code>";

    return out.str();
}

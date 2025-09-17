/*
date	Дата в формате YYYY-MM-DD
sunrise	Время восхода солнца — момент, когда верхний край Солнца появляется над горизонтом
sunset	Время заката солнца — момент, когда Солнце полностью исчезает за горизонтом
dawn	Астрономическая утренняя заря — когда солнце находится 6° под горизонтом (начало гражданских сумерек)
dusk	Астрономическая вечерняя заря — конец гражданских сумерек (Солнце на 6° под горизонтом)
solar_noon	Солнечный полдень — момент максимальной высоты Солнца над горизонтом
golden_hour	Начало "золотого часа" перед закатом — мягкий тёплый свет (примерно за 1 час до sunset)
day_length	Продолжительность дня — разница между sunset и sunrise
first_light	(часто null) — ориентировочное время, когда небо начинает светлеть до рассвета (на практике ≈ dawn)
last_light	(часто null) — ориентировочное время, когда небо окончательно темнеет после заката (≈ dusk)
timezone	Название часового пояса
utc_offset	Смещение в минутах от UTC (например, 180 → UTC+3:00)
*/

#include "SunriseSunset_io.hpp"

std::string getTodayDate() {
    time_t now = time(nullptr);
    tm* lt = localtime(&now);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return std::string(buf);
}

bool getTodaySunData(const std::string& filename, json& result) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    json j;
    file >> j;
    std::string today = getTodayDate();

    if (!j.contains("results")) return false;
    for (const auto& entry : j["results"]) {
        if (entry["date"] == today) {
            result = entry;
            return true;
        }
    }
    return false;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* stream = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    stream->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool downloadSunData(const std::string& url, const std::string& filename) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::ofstream file(filename);
    if (!file.is_open()) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    file.close();

    return (res == CURLE_OK);
}
std::string generateSunAPIUrl(double lat, double lng) {
    time_t now = time(nullptr);
    tm start_tm = *localtime(&now);

    std::ostringstream date_start;
    date_start << std::setfill('0') << std::setw(4) << (start_tm.tm_year + 1900) << "-"
               << std::setw(2) << (start_tm.tm_mon + 1) << "-"
               << std::setw(2) << start_tm.tm_mday;

    tm end_tm = start_tm;
    end_tm.tm_mon += 1;
    mktime(&end_tm);

    std::ostringstream date_end;
    date_end << std::setfill('0') << std::setw(4) << (end_tm.tm_year + 1900) << "-"
             << std::setw(2) << (end_tm.tm_mon + 1) << "-"
             << std::setw(2) << end_tm.tm_mday;

    std::ostringstream url;
    url << "https://api.sunrisesunset.io/json?"
        << "lat=" << lat
        << "&lng=" << lng
        << "&date_start=" << date_start.str()
        << "&date_end=" << date_end.str()
        << "&timezone=Europe/Moscow"
        << "&time_format=24";

    return url.str();
}

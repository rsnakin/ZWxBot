#pragma once

#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include <iomanip>
#include <ctime>

using json = nlohmann::json;

std::string getTodayDate();
bool getTodaySunData(const std::string& filename, json& result);
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
bool downloadSunData(const std::string& url, const std::string& filename);
std::string generateSunAPIUrl(double lat, double lng);

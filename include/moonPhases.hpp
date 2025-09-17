#pragma once

#include <string>
#include <vector>
#include <ctime>
#include "json.hpp"

using json = nlohmann::json;

std::vector<time_t> generateTimestamps(int days = 30);
std::string buildMoonPhaseURL();
bool downloadMoonPhaseJSON(const std::string& url, const std::string& filename);
bool getTodayMoonPhase(const std::string& filename, json& todayEntry);
std::string getMoonPhaseIcon(const std::string& phase);
std::string formatMoonPhaseInfo(const json& moon);

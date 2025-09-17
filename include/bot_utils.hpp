#pragma once

#include <string>

#define SHARED_MEMORY_OBJECT_SIZE 36
#define MEMORY_OBJECT_TEMPLATE "%1d%06lu%03d%10lu%10d%5s"

struct MeteoData {
    std::string user;
    double ds18b20_temp;
    double bmp180_pressure;
    int dht11_humidity;
    bool dht11_ok;
    const char* arrowT;
    const char* arrowP;
    const char* arrowH;
    std::string isRainLikely;
    std::string sunrise;
    std::string sunset;
    std::string day_length;
    std::string solar_noon;
    std::string moonPhase;
};

std::string execCmd(const char* cmd);
double readCpuTemp();
double readDS18B20();
bool readDHT11(int gpioPin, int& temp_c, int& hum);
std::string buildSystemStatus();
std::string decodeThrottledFlags(const std::string& hexStr);
std::string getWifiIcon(int rssi);
std::ostringstream meteoMsg(MeteoData& mData);
bool shmMemRead();

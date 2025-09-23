#pragma once

#define APP_NAME              "ZWxBot"
#define API_TEMP_DATA         "/api/temp_data"
#define API_CONF_DATA         "/api/config"
#define API_CONF_SAVE         "/api/config_save"
#define API_LOG               "/api/log"
#define API_DASHBOARD         "/api/dashboard"
#define LOG_PATH              "/run"
// DS18B20 -- GPIO4, BMP180 -- GPIO2 SDA, GPIO3	SCL
#define DHT11_GPIO            22
#define FAN_OBJECT_NAME       "FanPiPWMld"
#define TEMPERATURE_THRESHOLD 0.005f
#define PRESSURE_THRESHOLD    0.03f
#define HUMIDITY_THRESHOLD    0.01f
#define HUMIDITY_CORRECTION   1.27f
#define RAIN_LIKELY           "   &#x2614; <b>Rain is likely soon</b>"
#define NO_RAIN_EXP           "   &#x1F302; <b>No rain expected</b>"
#define RAIN_LIKELY_SHRT      "&#x2614; <b>Rain likely</b>"
#define NO_RAIN_EXP_SHRT      "&#x1F302; <b>Rain unlikely</b>"
#define SQLITE_RECORDS        2880
#define CFG_FILE              "/usr/local/etc/ZWxBot.json"
#define BASE_PATH             "/usr/local/share/ZWxBot"
#define WWW_PATH              BASE_PATH "/www"
#define SUNRISESUNSET_FILE    BASE_PATH "/sunrisesunset.json"
#define MOONPHASES_FILE       BASE_PATH "/moonphases.json"
#define BACKGROUND_PNG_FILE   BASE_PATH "/background.png"
#define SQLITE_DB_FILE        "/run/wx_data.db"
#define CHARTS_PNG_FILE       "/run/ZWxBotCharts.png"
#define CHART_PNG_NAME        "ZWxBotCharts.png"
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

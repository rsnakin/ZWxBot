cmake_minimum_required(VERSION 3.16)
project(rpi_env_monitor_bot LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -march=armv6 -mfpu=vfp -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -march=armv6 -mfpu=vfp -mfloat-abi=hard")

find_package(PkgConfig REQUIRED)

#pkg_check_modules(PIGPIO REQUIRED pigpio)         # GPIO bit-banging lib
#pkg_check_modules(I2C REQUIRED i2c)               # libi2c - SMBus helpers
find_package(Threads REQUIRED)                    # pthreads
find_package(OpenSSL REQUIRED)                    # SSL / crypto
find_package(ZLIB REQUIRED)                       # GZip / deflate
find_package(SQLite3 REQUIRED)

include_directories(include)
add_subdirectory(lib/ArduinoJson)
set(PROJECT_SRC
    src/main.cpp
    src/mongoose.c
    src/bot_utils.cpp
    src/bmp180.cpp
    src/TrendTracker.c
    src/log.cpp
    src/ssd1306.cpp
    src/correctHumidity.cpp
    src/SunriseSunset_io.cpp
    src/moonPhases.cpp
    src/sql.cpp
    src/lodepng.cpp
    src/charts.cpp
    src/version.cpp
)


include_directories(${SQLite3_INCLUDE_DIRS})

add_executable(ZWxBot
    ${PROJECT_SRC}
)

target_link_libraries(ZWxBot
    PRIVATE
        TgBot
        curl
        ssl
        crypto
        boost_system
        pthread
        pigpio
        i2c
        m
        rt
        /usr/local/lib/libu8g2.a
        ArduinoJson
        ${SQLite3_LIBRARIES}
)

target_include_directories(ZWxBot PRIVATE lib/ArduinoJson/src)

install(TARGETS ZWxBot RUNTIME DESTINATION bin)

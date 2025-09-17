#pragma once

#include <cstdint>
#include <linux/i2c-dev.h>

extern "C" {
    __s32 i2c_smbus_read_byte_data(int file, __u8 command);
    __s32 i2c_smbus_read_word_data(int file, __u8 command);
    __s32 i2c_smbus_write_byte_data(int file, __u8 command, __u8 value);
}

/************************************
 * Sensor: BMP180 minimal driver   *
 ***********************************/
class BMP180 {
    int fd{-1};
    // коэффициенты
    int16_t AC1, AC2, AC3, B1, B2, MB, MC, MD;
    uint16_t AC4, AC5, AC6;
    uint8_t  oss{0};                 // oversampling (0..3)

    uint8_t  read8(uint8_t r)               { return i2c_smbus_read_byte_data(fd, r); }
    uint16_t read16BE(uint8_t r)            { uint16_t v = i2c_smbus_read_word_data(fd, r);
                                              return (v << 8) | (v >> 8); }
    void     write8(uint8_t r, uint8_t v)   { i2c_smbus_write_byte_data(fd, r, v); }
public:
    bool begin();
    double readTemperature();
    double readPressure();
};

#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "bmp180.hpp"

bool BMP180::begin() {
    fd = open("/dev/i2c-1", O_RDWR);
    if (fd < 0) return false;
    if (ioctl(fd, I2C_SLAVE, 0x77) < 0) return false;

    AC1 = (int16_t)read16BE(0xAA);
    AC2 = (int16_t)read16BE(0xAC);
    AC3 = (int16_t)read16BE(0xAE);
    AC4 =          read16BE(0xB0);
    AC5 =          read16BE(0xB2);
    AC6 =          read16BE(0xB4);
    B1  = (int16_t)read16BE(0xB6);
    B2  = (int16_t)read16BE(0xB8);
    MB  = (int16_t)read16BE(0xBA);
    MC  = (int16_t)read16BE(0xBC);
    MD  = (int16_t)read16BE(0xBE);
    return true;
}
double BMP180::readTemperature() {
    write8(0xF4, 0x2E);
    usleep(5000);
    int32_t UT = read16BE(0xF6);

    int32_t X1 = ((UT - (int32_t)AC6) * (int32_t)AC5) >> 15;
    int32_t X2 = ((int32_t)MC << 11) / (X1 + (int32_t)MD);
    int32_t B5 = X1 + X2;
    return ((B5 + 8) >> 4) / 10.0;       // °C
}
double BMP180::readPressure() {

    write8(0xF4, 0x2E); usleep(5000);
    int32_t UT = read16BE(0xF6);
    int32_t X1 = ((UT - AC6) * AC5) >> 15;
    int32_t X2 = (MC << 11) / (X1 + MD);
    int32_t B5 = X1 + X2;

    write8(0xF4, 0x34 + (oss << 6));
    usleep(4500);
    int32_t UP = ((read8(0xF6) << 16) |
                    (read8(0xF7) << 8)  |
                    read8(0xF8)) >> (8 - oss);

    int32_t B6 = B5 - 4000;
    X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = (AC2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = (((AC1 * 4 + X3) << oss) + 2) >> 2;

    X1 = (AC3 * B6) >> 13;
    X2 = (B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = (AC4 * (uint32_t)(X3 + 32768)) >> 15;
    uint32_t B7 = ((uint32_t)UP - B3) * (50000 >> oss);

    int32_t p = (B7 < 0x80000000) ? (B7 * 2) / B4 : (B7 / B4) * 2;
    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p += (X1 + X2 + 3791) >> 4;

    return (p);
}


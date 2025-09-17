#include <pigpio.h>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include "ssd1306.hpp"

u8g2_t u8g2;
int pin_scl = SSD1306_SCL_GPIO;
int pin_sda = SSD1306_SDA_GPIO;
char scrBuffer[DROWS][DWIDTH] = {0};
size_t scrLines = 0;

extern "C" {
    uint8_t my_gpio_and_delay(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {

        switch (msg) {
            case U8X8_MSG_GPIO_AND_DELAY_INIT:
                gpioSetMode(pin_scl, PI_OUTPUT);
                gpioSetMode(pin_sda, PI_OUTPUT);
                return 1;
            case U8X8_MSG_GPIO_I2C_CLOCK:
                gpioWrite(pin_scl, arg_int);
                return 1;
            case U8X8_MSG_GPIO_I2C_DATA:
                gpioWrite(pin_sda, arg_int);
                return 1;
            case U8X8_MSG_DELAY_MILLI:
                usleep(arg_int * 1000);
                return 1;
            case U8X8_MSG_DELAY_I2C:
                return 1;
            case U8X8_MSG_DELAY_100NANO:
            case U8X8_MSG_DELAY_NANO:
                return 1;
            case U8X8_MSG_GPIO_RESET:
            case U8X8_MSG_GPIO_CS:
            case U8X8_MSG_GPIO_DC:
                return 1;
        }
        return 0;
    }
}
void initDisplay() {
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_sw_i2c,
        my_gpio_and_delay
    );

    u8x8_SetPin(&u8g2.u8x8, U8X8_PIN_I2C_CLOCK, SSD1306_SCL_GPIO); // SCL - GPIO27 pin 13
    u8x8_SetPin(&u8g2.u8x8, U8X8_PIN_I2C_DATA, SSD1306_SDA_GPIO);  // SDA - GPIO17 pin 11

    u8g2_SetI2CAddress(&u8g2, 0x78);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
}
void push_line(size_t *count, const char *line) {
	char temp[DWIDTH];
	strncpy(temp, line, DWIDTH - 1);
	temp[DWIDTH - 1] = '\0';

	if (*count < DROWS) {
		strcpy(scrBuffer[*count], temp);
		(*count)++;
	} else {
		memmove(scrBuffer[0], scrBuffer[1], (DROWS - 1) * DWIDTH);
		strcpy(scrBuffer[DROWS - 1], temp);
	}
}
void addMsg(const char *msg) {
	if(msg == nullptr) {
		memset(scrBuffer, 0, sizeof(scrBuffer));
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawBox(&u8g2, 1, 1, 126, 62);
        u8g2_SendBuffer(&u8g2);
		scrLines = 0;
		return;
	}
	push_line(&scrLines, msg);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
	for (size_t r = 0; r < scrLines; ++r) {
        u8g2_SetDrawColor(&u8g2, 0);
        u8g2_DrawBox(&u8g2, 2, r * 10 + 2, 123, 11);
        u8g2_SetDrawColor(&u8g2, 1);
        u8g2_DrawStr(&u8g2, 4, r * 10 + 11, scrBuffer[r]);
		if(r == (scrLines - 1)) {
            u8g2_SetDrawColor(&u8g2, 2);
            u8g2_DrawBox(&u8g2, 2, r * 10 + 2, 123, 10);
        }
	}
	u8g2_SendBuffer(&u8g2);
}
void setDisplayContrast(uint8_t contrast) {
    //u8g2_SendCommand(&u8g2, 0x81);
    //u8g2_SendCommand(&u8g2, contrast);
    u8x8_SendF(&u8g2.u8x8, "cc", 0x81, contrast);
}
int getCurrentHour() {
    time_t now = time(nullptr);
    struct tm *ltm = localtime(&now);
    return ltm->tm_hour;
}
uint8_t updateAutoBrightness() {
    int hour = getCurrentHour();
    uint8_t brightness;

    if (hour >= 7 && hour < 19) {
        brightness = 0xFF;
    } else if (hour >= 19 && hour < 22) {
        brightness = 0x80;
    } else {
        brightness = 0x20;
    }

    setDisplayContrast(brightness);
    return brightness;
}

#pragma once
#include <u8g2/u8g2.h>
#include <u8g2/u8x8.h>

#define DROWS                 6
#define DWIDTH                21
#define SSD1306_SCL_GPIO      27
#define SSD1306_SDA_GPIO      17

void initDisplay();
void push_line(size_t *count, const char *line);
void addMsg(const char *msg = nullptr);
void setDisplayContrast(uint8_t contrast);
int getCurrentHour();
uint8_t updateAutoBrightness();

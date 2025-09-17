#include <vector>
#include <cmath>
#include "json.hpp"
#include "lodepng.h"
#include "main.hpp"
#include "charts.hpp"
#include <mutex>

static std::vector<unsigned char> bgImage;
static unsigned bgWidth = 0, bgHeight = 0;
static std::once_flag bgInitFlag;

using json = nlohmann::json;

const uint8_t font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32: ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 33: !
    {0x00,0x07,0x00,0x07,0x00}, // 34: "
    {0x14,0x7F,0x14,0x7F,0x14}, // 35: #
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36: $
    {0x23,0x13,0x08,0x64,0x62}, // 37: %
    {0x36,0x49,0x55,0x22,0x50}, // 38: &
    {0x00,0x05,0x03,0x00,0x00}, // 39: '
    {0x1C,0x22,0x41,0x00,0x00}, // 40: (
    {0x00,0x41,0x22,0x1C,0x00}, // 41: )
    {0x14,0x08,0x3E,0x08,0x14}, // 42: *
    {0x08,0x08,0x3E,0x08,0x08}, // 43: +
    {0x00,0x50,0x30,0x00,0x00}, // 44: ,
    {0x08,0x08,0x08,0x08,0x08}, // 45: -
    {0x00,0x60,0x60,0x00,0x00}, // 46: .
    {0x20,0x10,0x08,0x04,0x02}, // 47: /
    {0x3E,0x51,0x49,0x45,0x3E}, // 48: 0
    {0x00,0x42,0x7F,0x40,0x00}, // 49: 1
    {0x42,0x61,0x51,0x49,0x46}, // 50: 2
    {0x21,0x41,0x45,0x4B,0x31}, // 51: 3
    {0x18,0x14,0x12,0x7F,0x10}, // 52: 4
    {0x27,0x45,0x45,0x45,0x39}, // 53: 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 54: 6
    {0x01,0x71,0x09,0x05,0x03}, // 55: 7
    {0x36,0x49,0x49,0x49,0x36}, // 56: 8
    {0x06,0x49,0x49,0x29,0x1E}, // 57: 9
    {0x00,0x36,0x36,0x00,0x00}, // 58: :
    {0x00,0x56,0x36,0x00,0x00}, // 59: ;
    {0x08,0x14,0x22,0x41,0x00}, // 60: <
    {0x14,0x14,0x14,0x14,0x14}, // 61: =
    {0x00,0x41,0x22,0x14,0x08}, // 62: >
    {0x02,0x01,0x51,0x09,0x06}, // 63: ?
    {0x32,0x49,0x79,0x41,0x3E}, // 64: @
    {0x7E,0x11,0x11,0x11,0x7E}, // 65: A
    {0x7F,0x49,0x49,0x49,0x36}, // 66: B
    {0x3E,0x41,0x41,0x41,0x22}, // 67: C
    {0x7F,0x41,0x41,0x22,0x1C}, // 68: D
    {0x7F,0x49,0x49,0x49,0x41}, // 69: E
    {0x7F,0x09,0x09,0x09,0x01}, // 70: F
    {0x3E,0x41,0x49,0x49,0x7A}, // 71: G
    {0x7F,0x08,0x08,0x08,0x7F}, // 72: H
    {0x00,0x41,0x7F,0x41,0x00}, // 73: I
    {0x20,0x40,0x41,0x3F,0x01}, // 74: J
    {0x7F,0x08,0x14,0x22,0x41}, // 75: K
    {0x7F,0x40,0x40,0x40,0x40}, // 76: L
    {0x7F,0x02,0x0C,0x02,0x7F}, // 77: M
    {0x7F,0x04,0x08,0x10,0x7F}, // 78: N
    {0x3E,0x41,0x41,0x41,0x3E}, // 79: O
    {0x7F,0x09,0x09,0x09,0x06}, // 80: P
    {0x3E,0x41,0x51,0x21,0x5E}, // 81: Q
    {0x7F,0x09,0x19,0x29,0x46}, // 82: R
    {0x46,0x49,0x49,0x49,0x31}, // 83: S
    {0x01,0x01,0x7F,0x01,0x01}, // 84: T
    {0x3F,0x40,0x40,0x40,0x3F}, // 85: U
    {0x1F,0x20,0x40,0x20,0x1F}, // 86: V
    {0x3F,0x40,0x38,0x40,0x3F}, // 87: W
    {0x63,0x14,0x08,0x14,0x63}, // 88: X
    {0x07,0x08,0x70,0x08,0x07}, // 89: Y
    {0x61,0x51,0x49,0x45,0x43}, // 90: Z
    {0x00,0x7F,0x41,0x41,0x00}, // 91: [
    {0x02,0x04,0x08,0x10,0x20}, // 92: '\'
    {0x00,0x41,0x41,0x7F,0x00}, // 93: ]
    {0x04,0x02,0x01,0x02,0x04}, // 94: ^
    {0x40,0x40,0x40,0x40,0x40}, // 95: _
    {0x00,0x01,0x02,0x04,0x00}, // 96: `
    {0x20,0x54,0x54,0x54,0x78}, // 97: a
    {0x7F,0x48,0x44,0x44,0x38}, // 98: b
    {0x38,0x44,0x44,0x44,0x20}, // 99: c
    {0x38,0x44,0x44,0x48,0x7F}, // 100: d
    {0x38,0x54,0x54,0x54,0x18}, // 101: e
    {0x08,0x7E,0x09,0x01,0x02}, // 102: f
    {0x0C,0x52,0x52,0x52,0x3E}, // 103: g
    {0x7F,0x08,0x04,0x04,0x78}, // 104: h
    {0x00,0x44,0x7D,0x40,0x00}, // 105: i
    {0x20,0x40,0x44,0x3D,0x00}, // 106: j
    {0x7F,0x10,0x28,0x44,0x00}, // 107: k
    {0x00,0x41,0x7F,0x40,0x00}, // 108: l
    {0x7C,0x04,0x18,0x04,0x78}, // 109: m
    {0x7C,0x08,0x04,0x04,0x78}, // 110: n
    {0x38,0x44,0x44,0x44,0x38}, // 111: o
    {0x7C,0x14,0x14,0x14,0x08}, // 112: p
    {0x08,0x14,0x14,0x18,0x7C}, // 113: q
    {0x7C,0x08,0x04,0x04,0x08}, // 114: r
    {0x48,0x54,0x54,0x54,0x20}, // 115: s
    {0x04,0x3F,0x44,0x40,0x20}, // 116: t
    {0x3C,0x40,0x40,0x20,0x7C}, // 117: u
    {0x1C,0x20,0x40,0x20,0x1C}, // 118: v
    {0x3C,0x40,0x30,0x40,0x3C}, // 119: w
    {0x44,0x28,0x10,0x28,0x44}, // 120: x
    {0x0C,0x50,0x50,0x50,0x3C}, // 121: y
    {0x44,0x64,0x54,0x4C,0x44}, // 122: z
    {0x00,0x08,0x36,0x41,0x00}, // 123: {
    {0x00,0x00,0x7F,0x00,0x00}, // 124: |
    {0x00,0x41,0x36,0x08,0x00}, // 125: }
    {0x10,0x08,0x08,0x10,0x08}, // 126: ~
    {0x06,0x09,0x09,0x06,0x00} // 96: ° (248)
};
void setPixel(std::vector<unsigned char>& img, unsigned w, unsigned h, 
    unsigned x, unsigned y, unsigned char r, unsigned char g, unsigned char b) {
    if (x >= w || y >= h) return;
    unsigned idx = 4 * (y * w + x);
    img[idx] = r;
    img[idx+1] = g;
    img[idx+2] = b;
    img[idx+3] = 255;
}
void drawLine(std::vector<unsigned char>& img, unsigned w, unsigned h, 
    int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        setPixel(img, w, h, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}
void drawChar(std::vector<unsigned char>& img, unsigned w, unsigned h, 
              int x, int y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c < 32 || (c > 127 && c != 176)) return;
    unsigned int c_; 
    if(c != 176) {
        c_ = c - 32;
    } else {
        c_ = 95;
    }
    const uint8_t* glyph = font5x7[c_];
    for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 7; ++row) {
            if (glyph[col] & (1 << row)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && py >= 0 && px < (int)w && py < (int)h) {
                    unsigned idx = 4 * (py * w + px);
                    img[idx] = r;
                    img[idx + 1] = g;
                    img[idx + 2] = b;
                    img[idx + 3] = 255;
                }
            }
        }
    }
}

void drawText(std::vector<unsigned char>& img, unsigned w, unsigned h, 
              int x, int y, const std::string& text, 
              unsigned char r = 0, unsigned char g = 0, unsigned char b = 0) {
    int cursorX = x;
    for (char c : text) {
        if (c == '\n') {
            y += 8;
            cursorX = x;
        } else {
            drawChar(img, w, h, cursorX, y, c, r, g, b);
            cursorX += 6;
        }
    }
}
void makeChart(const std::string& fileName, const std::string& jsonString, Log gLog) {
    const unsigned width = 800, height = 600;
    std::vector<unsigned char> image(width * height * 4);

    std::call_once(bgInitFlag, [&]() {
        unsigned error = lodepng::decode(bgImage, bgWidth, bgHeight, BACKGROUND_PNG_FILE);
        if (error) {
            gLog.write("[makeChart] warning: can't load '%s', using black background", BACKGROUND_PNG_FILE);
            bgImage.clear();
        } else if (bgWidth != width || bgHeight != height) {
            gLog.write("[makeChart] warning: '%s' size doesn't match (got %ux%u), using black background",
                       BACKGROUND_PNG_FILE, bgWidth, bgHeight);
            bgImage.clear();
        } else {
            gLog.write("[makeChart] background loaded from '%s'", BACKGROUND_PNG_FILE);
        }
    });

    if (!bgImage.empty()) {
        image = bgImage;
    } else {
        std::fill(image.begin(), image.end(), 0);
    }

    json j = json::parse(jsonString);
    size_t n = j.size();
    if (n < 2) {
        gLog.write("[makeChart] error: Not enough data points");
        return;
    }

    double minTemp = j[0]["v1"];
    double maxTemp = minTemp;
    double minPres = double(j[0]["v2"]) / 133.322;
    double maxPres = minPres;
    double minHum = j[0]["v3"];
    double maxHum = minHum;

    for (size_t i = 1; i < n; ++i) {
        double temp = j[i]["v1"];
        double pres = double(j[i]["v2"]) / 133.322;
        double hum = j[i]["v3"];

        if (temp < minTemp) minTemp = temp;
        if (temp > maxTemp) maxTemp = temp;

        if (pres < minPres) minPres = pres;
        if (pres > maxPres) maxPres = pres;

        if (hum < minHum) minHum = hum;
        if (hum > maxHum) maxHum = hum;
    }

    auto expandRange = [](double &minVal, double &maxVal, double padding = 0.05) {
        double range = maxVal - minVal;
        if (range == 0) range = 1.0;
        minVal -= range * padding;
        maxVal += range * padding;
    };

    expandRange(minTemp, maxTemp);
    expandRange(minPres, maxPres);
    expandRange(minHum, maxHum);

    int marginLeft = 80, marginRight = 20;
    int marginTop = 10, marginBottom = 20;
    int zoneMargin = 20;
    int zonesCount = 3;
    int plotHeight = height - marginTop - marginBottom;
    int zoneHeight = (plotHeight - zoneMargin * (zonesCount - 1)) / zonesCount;
    double xStep = (double)(width - marginLeft - marginRight) / (n - 1);

    auto scaleY = [&](double value, double minVal, double maxVal, int yOffset) -> int {
        return yOffset + zoneHeight - 1 - (int)(((value - minVal) / (maxVal - minVal)) * (zoneHeight - 1));
    };

    auto drawGraph = [&](auto getVal, double minVal, double maxVal, unsigned char r, unsigned char g, unsigned char b, int yOffset) {
        for (size_t i = 1; i < n; ++i) {
            int x0 = marginLeft + (int)((i - 1) * xStep);
            int x1 = marginLeft + (int)(i * xStep);
            double v0 = getVal(j[i - 1]);
            double v1 = getVal(j[i]);
            int y0 = scaleY(v0, minVal, maxVal, yOffset + marginTop);
            int y1 = scaleY(v1, minVal, maxVal, yOffset + marginTop);
            drawLine(image, width, height, x0, y0, x1, y1, r, g, b);
        }
    };

    auto drawGrid = [&](double minVal, double maxVal, int xOffset, int yOffset, const char *unit) {
        for (int i = 0; i <= 5; ++i) {
            double v = minVal + i * (maxVal - minVal) / 5.0;
            int y = scaleY(v, minVal, maxVal, yOffset + marginTop);
            drawLine(image, width, height, marginLeft, y, width - marginRight - 1, y, 80, 80, 80);
            char label[16];
            snprintf(label, sizeof(label), "%.1f%s", v, unit);
            drawText(image, width, height, xOffset, y - 5, label, 30, 30, 30);
        }
    };
    char grad[3];
    snprintf(grad, sizeof(grad), "%cC", 176);
    drawGrid(minTemp, maxTemp, 30, 0, grad);
    drawGrid(minPres, maxPres, 10, zoneHeight + zoneMargin, "mmHg");
    drawGrid(minHum, maxHum, 30, (zoneHeight + zoneMargin) * 2, "%");

    int gridCount = 6;
    for (int g = 0; g < gridCount; ++g) {
        size_t i = g * (n - 1) / (gridCount - 1);
        int x = marginLeft + (int)(i * xStep);
        std::string tstr = j[i]["t0"];
        std::string hhmm = tstr.substr(11, 5);

        drawLine(image, width, height, x, 
            marginTop, x, height - marginBottom - zoneMargin * 2 - zoneHeight * 2 - 3, 
            128, 128, 128);
        drawLine(image, width, height, x, 
            marginTop + zoneHeight + zoneMargin, x, height - marginBottom - zoneMargin - zoneHeight - 3, 
            128, 128, 128);
        drawLine(image, width, height, x, 
            marginTop + zoneHeight * 2 + zoneMargin * 2, x, height - marginBottom - 3, 
            128, 128, 128);

        drawText(image, width, height, x - 12, height - 14, hhmm, 30, 30, 30);
    }

    drawGraph([&](const json& d){ return double(d["v1"]); }, minTemp, maxTemp, 0, 255, 0, 0);
    drawGraph([&](const json& d){ return double(d["v2"]) / 133.322; }, minPres, maxPres, 0, 128, 255, zoneHeight + zoneMargin);
    drawGraph([&](const json& d){ return double(d["v3"]); }, minHum, maxHum, 64, 255, 255, (zoneHeight + zoneMargin) * 2);

    unsigned error = lodepng::encode(fileName, image, width, height);
    if (error)
        gLog.write("[makeChart] PNG encode error: %s", lodepng_error_text(error));

}

#pragma once

#include <vector>
#include "log.hpp"

void setPixel(std::vector<unsigned char>& img, unsigned w, unsigned h, 
    unsigned x, unsigned y, unsigned char r, unsigned char g, unsigned char b);
void drawLine(std::vector<unsigned char>& img, unsigned w, unsigned h, 
    int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b);
void makeChart(const std::string& fileName, const std::string& jsonString, Log gLog);

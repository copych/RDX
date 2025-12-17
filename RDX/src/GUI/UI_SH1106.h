#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "UI_Display.h"

class SH1106_Display : public UI_Display {
public:
    SH1106_Display(uint8_t addr = 0x3C) : addr_(addr) {
        width_  = 128;
        height_ = 64;
        buf_    = buffer_;
    }

    void begin() {
        Wire.begin();
        initDisplay();
        clear();
        update();
    }

    // --- core display info ---
    int getWidth()  const override { return width_; }
    int getHeight() const override { return height_; }

    // --- text / font ---
    void setFontByGliphSize(uint8_t w, uint8_t h) override {}
    int getTextWidth(const char* txt) override { return strlen(txt) * 6; } // dummy 6x8
    int getFontHeight() override { return 8; }


    // --- drawing core ---
    void clear() override {
        memset(buf_, 0x00, sizeof(buffer_));
    }

    void clearRegion(int x, int y, int w, int h) override {
        setColor(UIDisplayColor::BLACK);
        for (int yy = y; yy < y + h; ++yy) {
            drawHLine(x, yy, w);
        }
    }

    void update() override {
        for (uint8_t page = 0; page < height_ / 8; ++page) {
            sendCommand(0xB0 | page); // page address
            sendCommand(0x02);        // lower column start + offset 2
            sendCommand(0x10);        // higher column start

            Wire.beginTransmission(addr_);
            Wire.write(0x40);
            for (uint8_t col = 0; col < width_; ++col) {
                Wire.write(buf_[page * width_ + col]);
            }
            Wire.endTransmission();
        }
    }

    void setPixel(int x, int y, bool color) override {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        uint8_t page = y >> 3;
        uint8_t bit  = y & 7;
        if (color) buf_[page*width_ + x] |= 1 << bit;
        else       buf_[page*width_ + x] &= ~(1 << bit);
    }

    bool getPixel(int x, int y) const override {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
        uint8_t page = y >> 3;
        uint8_t bit  = y & 7;
        return (buf_[page*width_ + x] >> bit) & 1;
    }

    void drawCharPublic(int x, int y, char c) { drawChar(x, y, c); }

private:
    uint8_t addr_;
    uint8_t buffer_[128*8] = {0};

    void sendCommand(uint8_t cmd) {
        Wire.beginTransmission(addr_);
        Wire.write(0x00);
        Wire.write(cmd);
        Wire.endTransmission();
    }

    void initDisplay() {
        static const uint8_t init_seq[] = {
            0xAE, 0xD5, 0x80, 0xA8, 0x3F,
            0xD3, 0x00, 0x40, 0xAD, 0x8B,
            0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
            0xD9, 0x22, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
        };
        for (auto c : init_seq) sendCommand(c);
    }
};

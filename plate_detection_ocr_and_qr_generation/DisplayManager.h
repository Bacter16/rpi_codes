#pragma once

#include "Constants.h"
#include <string>
#include <cstdint>
#include <qrencode.h>

class DisplayManager {
public:
    DisplayManager() = default;
    ~DisplayManager();

    bool initialize();
    void clearScreen(uint16_t color = constants::WHITE);
    bool displayQRCode(const std::string& text);
    void setBacklight(bool on);

private:
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendData16(uint16_t data);
    void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillScreen(uint16_t color);
    void drawGeneratedQRCode(const QRcode* qrcode);
    void initDisplay();
};
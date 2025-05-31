#include "DisplayManager.h"
#include "Constants.h"

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <iostream>
#include <cmath>

bool DisplayManager::initialize() {
    if (constants::TFT_CS >= 0) pinMode(constants::TFT_CS, OUTPUT);
    pinMode(constants::TFT_DC, OUTPUT);
    if (constants::TFT_BLK >= 0) { pinMode(constants::TFT_BLK, OUTPUT); digitalWrite(constants::TFT_BLK, LOW); }
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, HIGH);

    if (wiringPiSPISetup(constants::SPI_CHANNEL, constants::SPI_SPEED) < 0) {
        std::cerr << "SPI setup failed" << std::endl;
        return false;
    }
    std::cout << "SPI setup OK (Channel " << constants::SPI_CHANNEL
              << ", Speed " << constants::SPI_SPEED << " Hz)." << std::endl;

    initDisplay();
    clearScreen(constants::WHITE);
    setBacklight(true);
    return true;
}

DisplayManager::~DisplayManager() {
    fillScreen(constants::BLACK);
    setBacklight(false);
    printf("Display cleaned up.\n");
}

void DisplayManager::clearScreen(uint16_t color) {
    fillRect(0, 0, constants::WIDTH, constants::HEIGHT, color);
}

void DisplayManager::fillScreen(uint16_t color) {
    fillRect(0, 0, constants::WIDTH, constants::HEIGHT, color);
}

bool DisplayManager::displayQRCode(const std::string& text) {
    if (text.empty()) return false;

    QRcode* qrcode = QRcode_encodeString(text.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (qrcode == NULL) {
        std::cerr << "Failed to generate QR code for: " << text << std::endl;
        return false;
    }

    printf("Displaying new QR code...\n");
    fillScreen(constants::BLACK);
    drawGeneratedQRCode(qrcode);
    QRcode_free(qrcode);
    return true;
}

void DisplayManager::setBacklight(bool on) {
    if (constants::TFT_BLK >= 0) {
        digitalWrite(constants::TFT_BLK, on ? HIGH : LOW);
    }
}

void DisplayManager::sendCommand(uint8_t cmd) {
    digitalWrite(constants::TFT_DC, LOW);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, LOW);
    wiringPiSPIDataRW(constants::SPI_CHANNEL, &cmd, 1);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, HIGH);
}

void DisplayManager::sendData(uint8_t data) {
    digitalWrite(constants::TFT_DC, HIGH);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, LOW);
    wiringPiSPIDataRW(constants::SPI_CHANNEL, &data, 1);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, HIGH);
}

void DisplayManager::sendData16(uint16_t data) {
    uint8_t buffer[2];
    buffer[0] = data >> 8; buffer[1] = data & 0xFF;
    digitalWrite(constants::TFT_DC, HIGH);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, LOW);
    wiringPiSPIDataRW(constants::SPI_CHANNEL, buffer, 2);
    if (constants::TFT_CS >= 0) digitalWrite(constants::TFT_CS, HIGH);
}

void DisplayManager::setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
 #define ST7789_CASET   0x2A
 #define ST7789_RASET   0x2B
 #define ST7789_RAMWR   0x2C
    uint16_t xs=x0, xe=x1, ys=y0, ye=y1;
    sendCommand(ST7789_CASET); sendData16(xs); sendData16(xe);
    sendCommand(ST7789_RASET); sendData16(ys); sendData16(ye);
    sendCommand(ST7789_RAMWR);
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if((x >= constants::WIDTH) || (y >= constants::HEIGHT)) return;
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
    if((x + w) > constants::WIDTH) w = constants::WIDTH - x;
    if((y + h) > constants::HEIGHT) h = constants::HEIGHT - y;
    if((w <= 0) || (h <= 0)) return;

    setAddressWindow(x, y, x + w - 1, y + h - 1);
    long pixelCount = (long)w * h;
    for (long i = 0; i < pixelCount; i++) {
        sendData16(color);
    }
}

void DisplayManager::drawGeneratedQRCode(const QRcode* qrcode) {
    if (!qrcode) { printf("Internal Error: Cannot draw NULL QR code struct.\n"); return; }
    int qrw = qrcode->width;
    int tw = qrw + 2 * constants::QUIET_ZONE_MODULES;
    int ms = floor((double)constants::WIDTH / tw); if (ms < 1) ms = 1;
    int ts = ms * tw;
    int xo = (constants::WIDTH - ts) / 2;
    int yo = (constants::HEIGHT - ts) / 2;
    printf("  Drawing QR: V=%d, W=%d, Mod=%dpx, Total=%dpx, Off=(%d,%d)\n", qrcode->version, qrw, ms, ts, xo, yo);
    int qx = xo + constants::QUIET_ZONE_MODULES * ms;
    int qy = yo + constants::QUIET_ZONE_MODULES * ms;
    for (int y = 0; y < qrw; y++) {
        for (int x = 0; x < qrw; x++) {
            if (qrcode->data[y * qrw + x] & 0x01) {
                fillRect(qx + x * ms, qy + y * ms, ms, ms, constants::WHITE);
            }
        }
    }
    printf("  QR Code drawing complete.\n");
}

void DisplayManager::initDisplay() {
 #define ST7789_SWRESET 0x01
 #define ST7789_COLMOD  0x3A
 #define ST7789_MADCTL  0x36
 #define ST7789_GMCTRP1 0xE0
 #define ST7789_GMCTRN1 0xE1
 #define ST7789_INVOFF  0x20
 #define ST7789_SLPOUT  0x11
 #define ST7789_DISPON  0x29
    printf("Initializing ST7789V display hardware...\n");
    if (constants::TFT_RST >= 0) {
        printf("  Hardware reset...\n");
        pinMode(constants::TFT_RST, OUTPUT);
        digitalWrite(constants::TFT_RST, HIGH); delay(50);
        digitalWrite(constants::TFT_RST, LOW); delay(50);
        digitalWrite(constants::TFT_RST, HIGH); delay(150);
    } else {
        printf("  Software Reset...\n");
        sendCommand(ST7789_SWRESET); delay(150);
    }
    printf("  Sending init commands...\n");
    sendCommand(ST7789_COLMOD); sendData(0x55); delay(10);
    sendCommand(ST7789_MADCTL); sendData(0x00);

    sendCommand(ST7789_GMCTRP1); sendData(0xD0); sendData(0x04); sendData(0x0D); sendData(0x11); sendData(0x13); sendData(0x2B); sendData(0x3F); sendData(0x54); sendData(0x4C); sendData(0x18); sendData(0x0D); sendData(0x0B); sendData(0x1F); sendData(0x23);
    sendCommand(ST7789_GMCTRN1); sendData(0xD0); sendData(0x04); sendData(0x0C); sendData(0x11); sendData(0x13); sendData(0x2C); sendData(0x3F); sendData(0x44); sendData(0x51); sendData(0x2F); sendData(0x1E); sendData(0x1E); sendData(0x1F); sendData(0x23);
    sendCommand(ST7789_INVOFF);
    sendCommand(ST7789_SLPOUT); delay(120);
    sendCommand(ST7789_DISPON); delay(50);
    printf("  Initialization complete.\n");
}
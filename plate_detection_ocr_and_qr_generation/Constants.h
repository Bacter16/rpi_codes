#pragma once

#include <cstdint>
#include <cstddef>

namespace constants {
    const int TFT_CS = 8;
    const int TFT_DC = 25;
    const int TFT_RST = 27;
    const int TFT_BLK = -1;
    const int SPI_CHANNEL = 0;
    const int SPI_SPEED = 32000000;
    const int WIDTH = 240;
    const int HEIGHT = 320;

    const uint16_t BLACK = 0x0000;
    const uint16_t WHITE = 0xFFFF;
    const uint16_t RED = 0xF800;

    const int QUIET_ZONE_MODULES = 4;

    const int REC_HEIGHT = 32;
    const int REC_WIDTH = 320;

    constexpr const char* LIBCAMERA_COMMAND = "libcamera-vid -t 0 --width 1080 --height 720 --framerate 30 --codec mjpeg --inline --nopreview --output -";
    extern const float MEAN_VALUES[3];
    extern const float STD_VALUES[3];

    const int MIN_PLATE_LENGTH = 5;
    const size_t MAX_JPEG_BUFFER = 2 * 1024 * 1024;

}
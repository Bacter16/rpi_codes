#include "Application.h"
#include <iostream>
#include <string>
#include <vector>
#include <signal.h>
#include <wiringPi.h>

bool exit_requested = false;

void signal_handler(int signal_num) {
    std::cout << "\nReceived signal " << signal_num << ", requesting exit." << std::endl;
    exit_requested = true;
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <det.param> <det.bin> <ocr.nb> <ocr.txt> [det_thresh] [char_conf]" << std::endl;
        std::cerr << "Example: " << argv[0] << " model.param model.bin ocr.nb dict.txt 0.4 0.1" << std::endl;
        return -1;
    }

    AppConfig config;
    config.detParam = argv[1];
    config.detBin = argv[2];
    config.ocrModel = argv[3];
    config.ocrLabel = argv[4];
    config.detThresh = (argc >= 6) ? std::stof(argv[5]) : 0.35f;
    config.charConf = (argc >= 7) ? std::stof(argv[6]) : 0.0f;

    if (wiringPiSetupGpio() == -1) {
        perror("WiringPi setup failed");
        return 1;
    }

    Application app(config);

    if (app.initialize()) {
        app.run();
    } else {
        std::cerr << "Application initialization failed." << std::endl;
        return 1;
    }

    std::cout << "Application finished gracefully." << std::endl;
    return 0;
}
#pragma once

#include "Constants.h"
#include "DisplayManager.h"
#include "CameraReader.h"
#include "LicensePlateDetector.h"
#include "TextRecognizer.h"
#include <string>

struct AppConfig {
    std::string detParam, detBin, ocrModel, ocrLabel;
    float detThresh = 0.35f;
    float charConf = 0.0f;
};

class Application {
public:
    Application(const AppConfig& cfg);
    ~Application() = default;

    bool initialize();

    void run();

private:
    AppConfig config;
    DisplayManager displayManager;
    CameraReader cameraReader;
    LicensePlateDetector detector;
    TextRecognizer recognizer;
    std::string currentlyDisplayedPlate = "";
};
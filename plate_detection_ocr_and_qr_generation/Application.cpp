#include "Application.h"
#include "Constants.h"
#include <iostream>
#include <chrono>
#include <thread>

extern bool exit_requested;

Application::Application(const AppConfig& cfg) :
    config(cfg),
    cameraReader(constants::LIBCAMERA_COMMAND)
{}

bool Application::initialize() {
    std::cout << "Initializing application components..." << std::endl;
    if (!displayManager.initialize()) {
        std::cerr << "Failed to initialize Display Manager." << std::endl;
        return false;
    }
    if (detector.init(config.detParam, config.detBin) != 0) {
         std::cerr << "Failed to initialize license plate detector." << std::endl;
         return false;
    }
    detector.set_confidence(config.detThresh);
    std::cout << "Detection threshold set to: " << config.detThresh << std::endl;

    if (recognizer.init(config.ocrModel, config.ocrLabel) != 0) {
         std::cerr << "Failed to initialize OCR recognizer." << std::endl;
         return false;
    }
    recognizer.set_char_confidence(config.charConf);

    if (!cameraReader.open()) {
        std::cerr << "Failed to open camera pipe." << std::endl;
        return false;
    }
    std::cout << "All components initialized successfully." << std::endl;
    return true;
}

void Application::run() {
    cv::Mat frame;
    std::cout << "Starting main application loop..." << std::endl;

    while (!exit_requested) {
        if (!cameraReader.readFrame(frame)) {
            if(exit_requested) break;
            std::cerr << "Failed to read frame, attempting to reopen pipe..." << std::endl;
            cameraReader.close();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!cameraReader.open()) {
                 std::cerr << "Failed to reopen pipe, exiting." << std::endl;
                 break;
            }
            continue;
        }

        std::vector<LicensePlateDetector::Object> plates = detector.detect(frame);

        if (!plates.empty()) {
            const auto& plate = plates[0];
            cv::Rect plate_rect_int(cv::Point(plate.rect.x, plate.rect.y),
                                   cv::Size(plate.rect.width, plate.rect.height));
            plate_rect_int &= cv::Rect(0, 0, frame.cols, frame.rows);

            if (plate_rect_int.width > 0 && plate_rect_int.height > 0) {
                cv::Mat plate_img = frame(plate_rect_int).clone();
                std::string ocr_result = recognizer.recognize(plate_img);

                if (ocr_result.length() >= constants::MIN_PLATE_LENGTH &&
                    ocr_result != currentlyDisplayedPlate)
                {
                    std::cout << "New Plate Detected: [" << ocr_result << "]" << std::endl;

                    if (displayManager.displayQRCode(ocr_result)) {
                         currentlyDisplayedPlate = ocr_result;
                    } else {
                        std::cerr << "Display failed, will retry detection." << std::endl;
                        currentlyDisplayedPlate = "";
                    }
                } else if (!ocr_result.empty() && ocr_result == currentlyDisplayedPlate) {
                    std::cout << "Same plate seen: [" << ocr_result << "]" << std::endl;
                }
            }
        }

    }

    std::cout << "Run loop finished." << std::endl;
}
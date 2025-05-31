#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "paddle_api.h"

class TextRecognizer {
public:
    TextRecognizer() = default;
    ~TextRecognizer() = default;

    int init(const std::string& model_path, const std::string& label_path);
    void set_char_confidence(float threshold);
    std::string recognize(const cv::Mat& plate_img);

private:
    std::shared_ptr<paddle::lite_api::PaddlePredictor> predictor;
    std::vector<std::string> labels;
    int num_classes = 0;
    float char_confidence_threshold = 0.0f;
};
#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

namespace OcrUtils {

std::vector<std::string> load_labels(const std::string& path);

void process_image(const cv::Mat& input_image,
                   std::vector<float>& input_data,
                   int height,
                   int width);

std::string ctc_decode(const float* softmax_data,
                       const std::vector<std::string>& labels,
                       int num_classes,
                       int length,
                       float char_confidence_threshold = 0.0f);

}
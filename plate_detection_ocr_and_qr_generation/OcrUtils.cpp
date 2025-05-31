#include "OcrUtils.h"
#include "Constants.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


namespace OcrUtils {

std::vector<std::string> load_labels(const std::string& path) {
    std::vector<std::string> labels;
    std::ifstream file(path);
    std::string line;
    if (!file.is_open()) {
        std::cerr << "Failed to open label file: " << path << std::endl;
        return labels;
    }
    while (std::getline(file, line)) {
        labels.push_back(line);
    }
    return labels;
}

void process_image(const cv::Mat& input_image, std::vector<float>& input_data, int height, int width) {
    if (input_image.empty()) {
        std::cerr << "Error: Empty image passed to process_image." << std::endl;
        input_data.assign(input_data.size(), 0.0f);
        return;
    }

    cv::Mat resized_image;
    cv::resize(input_image, resized_image, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    resized_image.convertTo(resized_image, CV_32FC3);

    int image_size = height * width;
    float* input_data_ptr = input_data.data();

    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            cv::Vec3f pixel_hwc = resized_image.at<cv::Vec3f>(h, w);
            input_data_ptr[0 * image_size + h * width + w] = (pixel_hwc[0] / 255.0f - constants::MEAN_VALUES[0]) / constants::STD_VALUES[0];
            input_data_ptr[1 * image_size + h * width + w] = (pixel_hwc[1] / 255.0f - constants::MEAN_VALUES[1]) / constants::STD_VALUES[1];
            input_data_ptr[2 * image_size + h * width + w] = (pixel_hwc[2] / 255.0f - constants::MEAN_VALUES[2]) / constants::STD_VALUES[2];
        }
    }
}


std::string ctc_decode(const float* softmax_data, const std::vector<std::string>& labels, int num_classes, int length, float char_confidence_threshold) {
    std::string result;
    int last_index = 0;

    for (int i = 0; i < length; ++i) {
        int argmax_index = 0;
        const float* current_step_data = softmax_data + i * num_classes;
        float max_value = current_step_data[0];

        for (int j = 1; j < num_classes; ++j) {
            if (current_step_data[j] > max_value) {
                max_value = current_step_data[j];
                argmax_index = j;
            }
        }

        if (argmax_index != 0 && argmax_index != last_index) {
             if (max_value >= char_confidence_threshold) {
                int label_index = argmax_index - 1;
                if (label_index >= 0 && label_index < labels.size()) {
                    result += labels[label_index];
                } else {
                     std::cerr << "Warning: CTC index out of bounds: " << label_index << std::endl;
                }
            }
        }
        last_index = argmax_index;
    }
    return result;
}

}
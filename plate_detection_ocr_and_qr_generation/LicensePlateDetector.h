#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "ncnn/net.h"

class LicensePlateDetector {
public:
    struct Object {
        cv::Rect_<float> rect;
        float prob;
    };

    LicensePlateDetector() = default;
    ~LicensePlateDetector() = default;

    int init(const std::string& param_path, const std::string& bin_path);

    void set_confidence(float conf_threshold);

    std::vector<Object> detect(const cv::Mat& frame);

private:
    ncnn::Net net;
    float confidence_threshold = 0.35f;

    float calculate_iou(const Object& a, const Object& b);
    std::vector<Object> apply_nms(const std::vector<Object>& objects, float nms_threshold = 0.45f);
};
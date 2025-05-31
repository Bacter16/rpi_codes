#include "LicensePlateDetector.h"
#include "ncnn/mat.h"
#include <iostream>
#include <algorithm>

int LicensePlateDetector::init(const std::string& param_path, const std::string& bin_path) {
    int ret = net.load_param(param_path.c_str());
    if (ret != 0) { std::cerr << "Error: Failed to load NCNN param file: " << param_path << std::endl; return -1; }
    ret = net.load_model(bin_path.c_str());
    if (ret != 0) { std::cerr << "Error: Failed to load NCNN bin file: " << bin_path << std::endl; return -1; }
    std::cout << "License plate detection model loaded successfully!" << std::endl; return 0;
}

void LicensePlateDetector::set_confidence(float conf_threshold) {
    confidence_threshold = conf_threshold;
}

std::vector<LicensePlateDetector::Object> LicensePlateDetector::detect(const cv::Mat& frame) {
    std::vector<Object> objects; const int target_size = 640; int img_w = frame.cols; int img_h = frame.rows;
    if (frame.empty()) { std::cerr << "Error: Empty frame passed to detector." << std::endl; return objects;}

    float scale_x = static_cast<float>(img_w) / target_size; float scale_y = static_cast<float>(img_h) / target_size;
    cv::Mat resized; cv::resize(frame, resized, cv::Size(target_size, target_size));
    ncnn::Mat in = ncnn::Mat::from_pixels(resized.data, ncnn::Mat::PIXEL_BGR2RGB, target_size, target_size);
    const float mean_vals[3] = {0.0f, 0.0f, 0.0f}; const float norm_vals[3] = {1/255.0f, 1/255.0f, 1/255.0f};
    in.substract_mean_normalize(mean_vals, norm_vals);
    ncnn::Extractor ex = net.create_extractor(); ex.input("in0", in); ncnn::Mat out;
    int ret = ex.extract("out0", out); if (ret != 0) { std::cerr << "Failed to extract NCNN output, error code: " << ret << std::endl; return objects; }

    for (int i = 0; i < out.w; i++) {
        float conf = out.row(4)[i]; if (conf > 1.0f) conf /= 255.0f;
        if (conf < confidence_threshold) continue;
        float cx = out.row(0)[i]; float cy = out.row(1)[i]; float w = out.row(2)[i]; float h = out.row(3)[i];
        if (w <= 0 || h <= 0 || cx < 0 || cy < 0) continue;
        float new_cx = cx * scale_x; float new_cy = cy * scale_y; float new_w = w * scale_x; float new_h = h * scale_y;
        int x1 = static_cast<int>(new_cx - new_w / 2); int y1 = static_cast<int>(new_cy - new_h / 2);
        int x2 = static_cast<int>(new_cx + new_w / 2); int y2 = static_cast<int>(new_cy + new_h / 2);
        x1 = std::max(0, std::min(x1, img_w - 1)); y1 = std::max(0, std::min(y1, img_h - 1));
        x2 = std::max(0, std::min(x2, img_w - 1)); y2 = std::max(0, std::min(y2, img_h - 1));
        if (x2 <= x1 || y2 <= y1) continue; if ((x2 - x1) < 20 || (y2 - y1) < 10) continue;
        if ((x2 - x1) > img_w * 0.9 || (y2 - y1) > img_h * 0.9) continue;
        Object obj; obj.rect = cv::Rect_<float>(x1, y1, x2 - x1, y2 - y1); obj.prob = conf; objects.push_back(obj);
    }
    objects = apply_nms(objects); return objects;
}

float LicensePlateDetector::calculate_iou(const Object& a, const Object& b) {
    float intersection_area = (a.rect & b.rect).area();
    float union_area = a.rect.area() + b.rect.area() - intersection_area;
    if (union_area <= 0) return 0;
    return intersection_area / union_area;
}

std::vector<LicensePlateDetector::Object> LicensePlateDetector::apply_nms(const std::vector<Object>& objects, float nms_threshold) {
    std::vector<Object> result; std::vector<int> indices(objects.size());
    for(int i=0; i<objects.size(); ++i) indices[i]=i;
    std::sort(indices.begin(), indices.end(), [&](int a, int b){ return objects[a].prob > objects[b].prob; });
    std::vector<bool> keep(objects.size(), true);
    for(int i=0; i<objects.size(); ++i) {
        if(!keep[indices[i]]) continue;
        for(int j=i+1; j<objects.size(); ++j) {
            if(!keep[indices[j]]) continue;
            float iou = calculate_iou(objects[indices[i]], objects[indices[j]]);
            if(iou > nms_threshold) keep[indices[j]] = false;
        }
    }
    for(int i=0; i<objects.size(); ++i) if(keep[indices[i]]) result.push_back(objects[indices[i]]);
    return result;
}
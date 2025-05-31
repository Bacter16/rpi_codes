#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <stdio.h>

class CameraReader {
public:
    CameraReader(const std::string& cmd);
    ~CameraReader();

    bool open();
    bool readFrame(cv::Mat& frame);
    void close();

private:
    std::string cameraCmd;
    FILE* pipe = nullptr;
    std::vector<uchar> buffer;
};
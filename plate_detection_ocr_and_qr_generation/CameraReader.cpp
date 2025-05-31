#include "CameraReader.h"
#include "Constants.h"
#include <iostream>
#include <vector>

CameraReader::CameraReader(const std::string& cmd) : cameraCmd(cmd), pipe(nullptr) {}

CameraReader::~CameraReader() {
    close();
}

bool CameraReader::open() {
    if (pipe) {
        return true;
    }
    pipe = popen(cameraCmd.c_str(), "r");
    if (!pipe) {
        perror("popen failed opening camera pipe");
        return false;
    }
    std::cout << "Camera pipe opened." << std::endl;

    return true;
}

void CameraReader::close() {
    if (pipe) {
        pclose(pipe);
        pipe = nullptr;
        std::cout << "Camera pipe closed." << std::endl;
    }
}

bool CameraReader::readFrame(cv::Mat& frame) {
    if (!pipe || feof(pipe)) {
        if(pipe && feof(pipe)) {
            std::cerr << "Camera pipe closed (EOF reached)." << std::endl;
            close();
        } else if (!pipe) {
             std::cerr << "Camera pipe not open." << std::endl;
        }
        return false;
    }

    uchar header[2];
    size_t header_read = fread(header, 1, 2, pipe);
    if (header_read != 2) {
         if(feof(pipe)) { close(); return false;}
         std::cerr << "Warning: Failed to read JPEG header fully (" << header_read << " bytes)." << std::endl;
         return false;
    }
    if (header[0] != 0xFF || header[1] != 0xD8) {
         return false;
    }

    buffer.assign({header[0], header[1]});
    uchar byte;
    bool foundEOI = false;
    size_t bytes_read_total = 0;

    while (fread(&byte, 1, 1, pipe) == 1) {
        buffer.push_back(byte);
        bytes_read_total++;
        if (buffer.size() >= 2 && buffer[buffer.size()-2] == 0xFF && buffer[buffer.size()-1] == 0xD9) {
            foundEOI = true;
            break;
        }
        if (buffer.size() > constants::MAX_JPEG_BUFFER) {
            std::cerr << "Warning: JPEG buffer limit exceeded." << std::endl;
            buffer.clear();
            return false;
        }
    }

    if (!foundEOI) {
        if (feof(pipe)) { std::cerr << "Camera pipe closed unexpectedly during frame read." << std::endl; close();}
        else { perror("fread error reading frame data from pipe"); }
        buffer.clear();
        return false;
    }

    try {
        frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
    } catch (const cv::Exception& e) {
        std::cerr << "cv::imdecode error: " << e.what() << std::endl;
        return false;
    }

    if (frame.empty()) {
        std::cerr << "Failed to decode frame (imdecode returned empty)." << std::endl;
        return false;
    }
    return true;
}
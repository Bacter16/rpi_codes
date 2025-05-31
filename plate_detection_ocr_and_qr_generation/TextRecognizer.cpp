#include "TextRecognizer.h"
#include "OcrUtils.h"
#include "Constants.h"
#include <iostream>
#include <vector>

int TextRecognizer::init(const std::string& model_path, const std::string& label_path) {
    labels = OcrUtils::load_labels(label_path);
    if (labels.empty()) {
        std::cerr << "Error: No labels loaded from: " << label_path << std::endl;
        return -1;
    }
    num_classes = labels.size() + 1;

    paddle::lite_api::MobileConfig config;
    config.set_model_from_file(model_path);
    config.set_threads(4);
    config.set_power_mode(paddle::lite_api::PowerMode::LITE_POWER_HIGH);

    try {
        predictor = paddle::lite_api::CreatePaddlePredictor<paddle::lite_api::MobileConfig>(config);
        if (predictor == nullptr) {
             std::cerr << "Error: Failed to create OCR predictor (nullptr)." << std::endl;
             return -1;
        }
        std::cout << "OCR recognition model loaded successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception while creating OCR predictor: " << e.what() << std::endl;
        return -1;
    }
}

void TextRecognizer::set_char_confidence(float threshold) {
    char_confidence_threshold = threshold;
    std::cout << "OCR character confidence threshold set to: " << threshold << std::endl;
}

std::string TextRecognizer::recognize(const cv::Mat& plate_img) {
    if (plate_img.empty()) {
        return "";
    }
    if (predictor == nullptr) {
         std::cerr << "Error: OCR predictor not initialized." << std::endl;
         return "";
    }

    try {
        std::vector<float> input_data(3 * constants::REC_HEIGHT * constants::REC_WIDTH);
        OcrUtils::process_image(plate_img, input_data, constants::REC_HEIGHT, constants::REC_WIDTH);

        auto input_tensor = predictor->GetInput(0);
        input_tensor->Resize({1, 3, constants::REC_HEIGHT, constants::REC_WIDTH});
        input_tensor->CopyFromCpu(input_data.data());

        predictor->Run();

        auto output_tensor = predictor->GetOutput(0);
        std::vector<int64_t> output_shape = output_tensor->shape();

        if (output_shape.size() != 3 || output_shape[0] != 1) {
             std::cerr << "Error: Unexpected OCR output shape." << std::endl;
             return "";
        }

        int64_t total_size = 1;
        for (auto dim : output_shape) total_size *= dim;
        if (total_size <= 0) {
             std::cerr << "Error: Invalid OCR output tensor size." << std::endl;
             return "";
        }

        std::vector<float> output_data(total_size);
        output_tensor->CopyToCpu(output_data.data());

        int seq_length = static_cast<int>(output_shape[1]);
        int output_num_classes = static_cast<int>(output_shape[2]);

        if (output_num_classes != num_classes) {
             std::cerr << "Warning: Model output classes (" << output_num_classes
                       << ") != Expected classes (" << num_classes << ")" << std::endl;
        }


        std::string result = OcrUtils::ctc_decode(output_data.data(), labels, output_num_classes, seq_length, char_confidence_threshold);
        return result;

    } catch (const std::exception& e) {
        std::cerr << "Exception during OCR recognition: " << e.what() << std::endl;
        return "";
    }
}
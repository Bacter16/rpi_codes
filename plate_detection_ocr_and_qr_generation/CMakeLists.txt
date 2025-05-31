cmake_minimum_required(VERSION 3.10)

project(lpr_qr_display)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fopenmp")

find_package(OpenCV REQUIRED)

set(NCNN_DIR "/usr/local")
set(NCNN_INCLUDE_DIR "${NCNN_DIR}/include")
set(NCNN_LIBRARY_DIR "${NCNN_DIR}/lib/ncnn") # Your working path

set(PADDLE_LITE_DIR "/home/bacter/paddle/Paddle-Lite")

set(PADDLE_LITE_LIB_DIR "${PADDLE_LITE_DIR}/build.lite.linux.armv8.gcc/inference_lite_lib.armlinux.armv8/cxx/lib")
set(PADDLE_LITE_INCLUDE_DIR "${PADDLE_LITE_DIR}/build.lite.linux.armv8.gcc/inference_lite_lib.armlinux.armv8/cxx/include")

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${NCNN_INCLUDE_DIR}
    ${PADDLE_LITE_INCLUDE_DIR}
    # Add standard paths where wiringPi/qrencode headers likely are
    /usr/local/include
    /usr/include
)

link_directories(
    ${NCNN_LIBRARY_DIR}
    ${PADDLE_LITE_LIB_DIR} # Use consistent name
    # Standard paths like /usr/lib, /usr/local/lib are searched by default
)

add_executable(lpr_qr_display
    main.cpp
    Application.cpp
    DisplayManager.cpp
    CameraReader.cpp
    LicensePlateDetector.cpp
    TextRecognizer.cpp
    OcrUtils.cpp
    Constants.cpp
)

target_link_libraries(lpr_qr_display
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
    opencv_highgui
    ncnn
    paddle_light_api_shared
    -lwiringPi
    -lqrencode
)

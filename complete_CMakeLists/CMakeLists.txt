cmake_minimum_required(VERSION 3.10)
project(SmartParkingSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fopenmp")

# Set NCNN paths - using the working example paths
set(NCNN_DIR "/usr/local")
set(NCNN_INCLUDE_DIR "${NCNN_DIR}/include")
set(NCNN_LIBRARY_DIR "${NCNN_DIR}/lib/ncnn")

# Set Paddle Lite paths
set(PADDLE_LITE_DIR "/home/bacter/paddle/Paddle-Lite")
set(PADDLE_LITE_LIB_DIR "${PADDLE_LITE_DIR}/build.lite.linux.armv8.gcc/inference_lite_lib.armlinux.armv8/cxx/lib")
set(PADDLE_LITE_INCLUDE_DIR "${PADDLE_LITE_DIR}/build.lite.linux.armv8.gcc/inference_lite_lib.armlinux.armv8/cxx/include")

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(QRENCODE REQUIRED libqrencode)
pkg_check_modules(CURL REQUIRED libcurl)

# Check for nlohmann json (for NetworkClient)
find_package(nlohmann_json 3.2.0 QUIET)
if (NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found, downloading from GitHub...")
  include(FetchContent)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(json)
endif()

# Include directories - added /usr/include and /usr/local/include explicitly
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
    ${QRENCODE_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${NCNN_INCLUDE_DIR}
    ${PADDLE_LITE_INCLUDE_DIR}
    /usr/local/include
    /usr/include
)

# Link directories
link_directories(
    ${NCNN_LIBRARY_DIR}
    ${PADDLE_LITE_LIB_DIR}
    /usr/local/lib
    /usr/lib
)

# Define source files
set(SOURCES
    src/main.cpp
    src/GpioManager.cpp
    src/UltrasonicSensor.cpp
    src/ServoMotor.cpp
    src/GateController.cpp
    src/DisplayManager.cpp
    src/CameraManager.cpp
    src/ALPRProcessor.cpp
    src/NetworkClient.cpp
    src/QRGenerator.cpp
    src/VehicleProcessor.cpp
    src/ParkingController.cpp
)

# Define header files
set(HEADERS
    src/Config.h
    src/Utils.h
    src/Logger.h
    src/GpioManager.h
    src/UltrasonicSensor.h
    src/ServoMotor.h
    src/GateController.h
    src/DisplayManager.h
    src/CameraManager.h
    src/ALPRProcessor.h
    src/NetworkClient.h
    src/QRGenerator.h
    src/VehicleProcessor.h
    src/ParkingController.h
        src/ServoMotorHW.cpp
        src/ServoMotorHW.h
        src/ServoMotorOptimized.cpp
        src/ServoMotorOptimized.h
)

# Create executable
add_executable(smart_parking_system ${SOURCES} ${HEADERS})

# Link libraries - using direct -l format for wiringPi and others
target_link_libraries(smart_parking_system
    ${OpenCV_LIBS}
    ${QRENCODE_LIBRARIES}
    ${CURL_LIBRARIES}
    ncnn
    paddle_light_api_shared
    -lgomp
    -lpthread
    -lwiringPi
    -llgpio
    nlohmann_json::nlohmann_json
)

# Installation rules
install(TARGETS smart_parking_system DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/models/ DESTINATION share/smart_parking_system/models/)

# Create a models directory in the build directory
add_custom_command(
    TARGET smart_parking_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/models
)

# Copy model files to the build directory if they exist
add_custom_command(
    TARGET smart_parking_system POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/models ${CMAKE_BINARY_DIR}/models
    COMMENT "Copying models to build directory"
)

# Create a directory for model files
install(CODE "file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/smart_parking_system/models)")

# Add a custom target to run the program with sudo
add_custom_target(
    run
    COMMAND sudo ${CMAKE_BINARY_DIR}/smart_parking_system
    DEPENDS smart_parking_system
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Output build information
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "Using C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "NCNN library dir: ${NCNN_LIBRARY_DIR}")
message(STATUS "Paddle Lite include: ${PADDLE_LITE_INCLUDE_DIR}")
message(STATUS "Paddle Lite libs: ${PADDLE_LITE_LIB_DIR}")
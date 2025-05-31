# Real-Time License Plate OCR and QR Code Display on Raspberry Pi

## Project Description

This project transforms a Raspberry Pi 5 equipped with a Camera Module 3 and an SPI display into a real-time License Plate Recognition (LPR) and QR code display system. It continuously monitors the video feed from the camera, utilizing a YOLOv8 model (via NCNN) to detect potential license plate regions within the frame. Once a plate is detected with sufficient confidence (>= 85%), the specific region is cropped and passed to an OCR model (via Paddle Lite) to recognize the alphanumeric characters. If a valid license plate string (>= 5 characters long) is consistently identified over several frames (debounce logic), the system generates a QR code encoding that plate number using libqrencode and displays it prominently on the connected SPI TFT screen (black background, white QR modules) for 15 seconds, pausing further detection during this time. After the display period, the screen is cleared (white background), and the system resumes scanning the camera feed for the next valid license plate.

## Schematics

This section describes the physical connections. Using different colored jumper wires is recommended for easier tracing. Ensure wires are routed neatly to avoid overlapping and potential shorts.

**Components:**

* Raspberry Pi 5
* ST7789V 2.0" SPI TFT Display (240x320)
* Raspberry Pi Camera Module 3
* Jumper Wires

**Connections:**

1.  **Camera:** Connect the Camera Module 3 to the Raspberry Pi 5's CSI port using its ribbon cable. Ensure correct orientation (blue tab usually faces away from the PCB clips).
2.  **SPI Display:** Connect the display to the Raspberry Pi 5's GPIO header using BCM Pin Numbering:

    | Display Pin Label | RPi Pin (BCM #) | RPi Pin (Physical #) | Wire Color Suggestion | Description                     |
    | :---------------- | :-------------- | :------------------- | :-------------------- | :------------------------------ |
    | VCC               | 3.3V or 5V      | 1 or 2 / 4           | Red                   | Power (Check display voltage!)  |
    | GND               | GND             | 6 / 9 / 14 / etc.    | Black                 | Ground                          |
    | SCL               | GPIO 11         | 23                   | Yellow                | SPI Clock (SPI0_SCLK)           |
    | SDA               | GPIO 10         | 19                   | Blue                  | SPI Data Out (SPI0_MOSI)        |
    | CS                | GPIO 8          | 24                   | Orange                | SPI Chip Select (SPI0_CE0_N)    |
    | DC                | GPIO 25         | 22                   | Green                 | Data/Command Select             |
    | RST               | GPIO 27         | 13                   | Purple                | Reset (Use -1 in code if unused)|

    * **Verify VCC:** Check if your display needs 3.3V (Pin 1 or 17) or 5V (Pin 2 or 4). Connect accordingly.
    * **GND:** Ensure a common ground connection.
    * **Optional Pins:** If your display lacks RST or BLK, set the corresponding defines (`TFT_RST`, `TFT_BLK`) in `Constants.h` to `-1`. If it lacks CS (tied low internally), set `TFT_CS` to `-1`.

*(For a visual diagram, consider using software like Fritzing: [https://fritzing.org/](https://fritzing.org/))*

## Prerequisites

### Hardware

* **Raspberry Pi:** **Model 5**.
    * [https://www.raspberrypi.com/products/raspberry-pi-5/](https://www.raspberrypi.com/products/raspberry-pi-5/)
* **SPI TFT Display:** **2.0 inch LCD, ST7789V driver, 240x320 resolution, SPI.**
    * *(Example Search: "ST7789V 240x320 2.0 SPI display" on major suppliers)*
* **Camera Module:** **Raspberry Pi Camera Module 3.**
    * [https://www.raspberrypi.com/products/camera-module-3/](https://www.raspberrypi.com/products/camera-module-3/)
* **MicroSD Card:** **128GB+ Class 10 recommended.**
* **Power Supply:** Official Raspberry Pi 27W USB-C Power Supply recommended for Pi 5.
* **Jumper Wires:** Female-to-Female Dupont wires.

### Software

* **OS:** Raspberry Pi OS (64-bit required for Pi 5, Bookworm recommended). Fully updated.
    * [https://www.raspberrypi.com/software/](https://www.raspberrypi.com/software/)
* **Build Tools:** Essential compiler tools, CMake, and pkg-config.
    ```bash
    sudo apt-get update && sudo apt-get install build-essential cmake pkg-config git
    ```
* **OpenCV:** Image processing library. **Install using the guide you followed:**
    * [https://qengineering.eu/install%20opencv%20on%20raspberry%20pi%205.html](https://qengineering.eu/install%20opencv%20on%20raspberry%20pi%205.html)
* **WiringPi:** GPIO control library (needs manual installation).
    * Clone/download from source (e.g., `git clone https://github.com/WiringPi/WiringPi`)
    * Follow its instructions (typically `cd WiringPi && sudo ./build`).
    * [http://wiringpi.com/](http://wiringpi.com/) (Original site)
* **libqrencode:** QR code generation library.
    ```bash
    sudo apt-get install libqrencode-dev
    ```
    * [https://github.com/fukuchi/libqrencode](https://github.com/fukuchi/libqrencode)
* **NCNN:** AI inference library (for detection). **Install using the guide you followed:**
    * [https://qengineering.eu/ncnn_rpi5.html](https://qengineering.eu/ncnn_rpi5.html)
    * *(Ensure OpenMP support was enabled during its build). Note the installation path.*
* **Paddle Lite:** AI inference library (for OCR). **Install using the guide you followed:**
    * [https://qengineering.eu/install-paddle-lite-on-raspberry-pi-4.html](https://qengineering.eu/install-paddle-lite-on-raspberry-pi-4.html)
    * *(Note the path to the final `inference_lite_lib.armlinux.armv8` directory).*
* **Models & Dictionary:** You must provide your own:
    * YOLOv8 NCNN detection model (`model.ncnn.param`, `model.ncnn.bin`).
    * Paddle Lite OCR model (`model_opt.nb`).
    * OCR character dictionary (`en_dict.txt`).

## Setup and Build

1.  **Prepare OS & Hardware:** Ensure Raspberry Pi OS (Bookworm 64-bit recommended) is installed and up-to-date. Enable the Camera interface (`sudo raspi-config` -> Interface Options -> Camera -> Enable; Disable legacy camera support). Connect display and camera per **Schematics**.
2.  **Install Apt Dependencies:** Install `build-essential`, `cmake`, `pkg-config`, `git`, `libqrencode-dev`.
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential cmake pkg-config git libqrencode-dev
    ```
3.  **Install OpenCV:** Follow the specific Q-engineering guide linked in **Prerequisites**.
4.  **Install WiringPi:** Clone the source and run `sudo ./build` as described in **Prerequisites**. Test with `gpio -v`.
5.  **Install NCNN:** Follow the specific Q-engineering guide linked in **Prerequisites**, ensuring OpenMP support is included. Verify the install location (likely `/usr/local`).
6.  **Install/Place Paddle Lite:** Follow the specific Q-engineering guide linked in **Prerequisites**. Note the path to the resulting `inference_lite_lib` directory.
7.  **Get Project Code:** Place all the project source files (`.h`, `.cpp`, `CMakeLists.txt`, `Constants.cpp`) into a single project directory (e.g., `~/lpr_qr_project`).
8.  **Place Models:** Create a `models` subdirectory (or similar) within `~/lpr_qr_project` and place your `.param`, `.bin`, `.nb`, and `.txt` files inside it.
9.  **Configure CMake (`CMakeLists.txt`):**
    * Open the `CMakeLists.txt` file provided with the project code.
    * **Verify Paths:** Carefully check the paths set for `NCNN_DIR`, `NCNN_LIBRARY_DIR`, and `PADDLE_LITE_DIR`. Ensure they match *your* specific installation/build locations from the Q-engineering guides. Adjust if necessary.
10. **Build:**
    ```bash
    # Navigate to the project directory
    cd ~/android_things_individual_project

    # Create and enter a build directory
    mkdir -p build
    cd build

    # Configure the project using CMake
    cmake ..

    # Compile the project (use -j4 for parallel build on Pi 5)
    make -j4
    ```
    If successful, the executable `lpr_qr_display` will be in the `build` directory.

## Running

1.  **Navigate** to the `build` directory:
    ```bash
    cd ~/lpr_qr_project/build
    ```
2.  **Run** the executable using `sudo` (for hardware access) and provide the full paths to your models and dictionary.

    **Command Format:**
    ```bash
    sudo ./lpr_qr_display <path/to/detection.param> <path/to/detection.bin> <path/to/ocr_model.nb> <path/to/ocr_labels.txt> [detection_threshold] [char_confidence]
    ```

    **Example:**
    ```bash
    sudo ./lpr_qr_display ../yolo_model/model.ncnn.param ../yolo_model/model.ncnn.bin ../paddle_ocr_model/ocr_opt.nb ../dict.txt 0.35 0.7
    ```
    * Replace example paths/thresholds with yours.
    * `detection_threshold` (e.g., `0.35`): General NCNN detection confidence.
    * `char_confidence` (e.g., `0.7`): Minimum confidence for OCR characters.
    * The 0.85 detection confidence threshold to *trigger the QR display* is currently hardcoded in `Constants.h`.

3.  **Operation:**
    * Initializes, clears display (white), analyzes camera feed.
    * Upon stable detection (>= 85% conf) and recognition (>= 5 chars), clears display (black) and shows QR code (white modules) for 15 seconds.
    * After 15s, clears display (white) and resumes detection.
4.  **Stopping:** Press `Ctrl+C` in the terminal. Display should clear black.
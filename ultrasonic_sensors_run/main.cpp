#include <iostream>     // For console input/output (std::cout, std::cerr)
#include <stdexcept>    // For standard exception types (std::runtime_error)
#include <chrono>       // For time durations
#include <thread>       // For pausing execution (std::this_thread::sleep_for)
#include <csignal>      // For signal handling (SIGINT - Ctrl+C)
#include <cmath>        // For distance calculation
#include <string>       // For std::to_string, std::string used in error messages

// Wrap the C library header in extern "C" for C++ compatibility
extern "C" {
    #include <lgpio.h>  // The C library for Raspberry Pi GPIO control
}

// --- Constants ---
// CORRECTED based on gpioinfo output
const int GPIO_CHIP = 0; // <<<--- Set to 0 based on gpioinfo output

// Define GPIO pins (BCM numbering) - Using Sensor 1 by default
// const int TRIG_PIN = 17; // Physical Pin 11
// const int ECHO_PIN = 27; // Physical Pin 13

// Sensor 2
// const int TRIG_PIN = 22; // Physical Pin 15
// const int ECHO_PIN = 5;  // Physical Pin 29 (VIA VOLTAGE DIVIDER!)

// Sensor 3
// const int TRIG_PIN = 26; // Physical Pin 37
// const int ECHO_PIN = 6;  // Physical Pin 31 (VIA VOLTAGE DIVIDER!)

// Sensor 4
const int TRIG_PIN = 20; // Physical Pin 38
const int ECHO_PIN = 21; // Physical Pin 40 (VIA VOLTAGE DIVIDER!)

const int TRIGGER_PULSE_US = 15; // Duration of trigger pulse in microseconds (>= 10us)
const int MEASUREMENT_INTERVAL_MS = 200; // Delay between measurements
const int POLLING_TIMEOUT_US = 40000; // Max time to wait for echo pulse (slightly > 38ms)

// --- Global Variables ---
volatile sig_atomic_t shutdown_flag = 0; // Flag set to 1 when Ctrl+C is pressed
int gpio_handle = -1; // Stores the handle for the opened GPIO chip

// --- Signal Handler Function ---
void handle_signal(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nCtrl+C detected. Shutting down..." << std::endl;
        shutdown_flag = 1;
    }
}

// --- Helper function to get current time in microseconds ---
// Note: Using std::chrono for potentially better precision than lgUptime
uint64_t micros_since_epoch() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}


// --- Main Function ---
int main() {
    signal(SIGINT, handle_signal); // Setup signal handling

    try {
        std::cout << "Initializing GPIO (Polling Method)..." << std::endl;
        // Try opening the CORRECT chip first based on gpioinfo
        gpio_handle = lgGpiochipOpen(GPIO_CHIP);
        if (gpio_handle < 0) {
             // If it fails, something else is wrong (permissions, library?)
             throw std::runtime_error("Failed to open GPIO chip " + std::to_string(GPIO_CHIP) + ". Error: " + std::string(lguErrorText(gpio_handle)));
        }
        std::cout << "Opened GPIO chip " << GPIO_CHIP << "." << std::endl;


        // --- Claim GPIO Pins ---
        int ret;
        ret = lgGpioClaimOutput(gpio_handle, 0, TRIG_PIN, 0); // Claim Trigger as output
        if (ret != LG_OKAY) throw std::runtime_error("Failed to claim Trigger pin " + std::to_string(TRIG_PIN) + ". Error: " + std::string(lguErrorText(ret)));
        std::cout << "Claimed GPIO " << TRIG_PIN << " for output." << std::endl;

        ret = lgGpioClaimInput(gpio_handle, 0, ECHO_PIN); // Claim Echo as input
         if (ret != LG_OKAY) {
            lgGpioFree(gpio_handle, TRIG_PIN);
            throw std::runtime_error("Failed to claim Echo pin " + std::to_string(ECHO_PIN) + " for input. Error: " + std::string(lguErrorText(ret)));
        }
        std::cout << "Claimed GPIO " << ECHO_PIN << " for input." << std::endl;

        std::cout << "Starting ultrasonic measurements (Polling). Press Ctrl+C to exit." << std::endl;
        std::cout << "Make sure the voltage divider is connected to the Echo pin!" << std::endl;

        // --- Measurement Loop ---
        while (!shutdown_flag) {
            // --- Send Trigger Pulse ---
            lgGpioWrite(gpio_handle, TRIG_PIN, 1);
            std::this_thread::sleep_for(std::chrono::microseconds(TRIGGER_PULSE_US));
            lgGpioWrite(gpio_handle, TRIG_PIN, 0);

            // --- Wait for Echo to Start (Go High) ---
            uint64_t startTime = micros_since_epoch();
            uint64_t timeLimitStart = startTime + POLLING_TIMEOUT_US; // Timeout limit
            while (lgGpioRead(gpio_handle, ECHO_PIN) == 0 && !shutdown_flag) {
                if (micros_since_epoch() > timeLimitStart) break; // Timeout waiting for start
            }

            // If timed out waiting for start, report and continue loop
            if (lgGpioRead(gpio_handle, ECHO_PIN) == 0 || shutdown_flag) {
                if (!shutdown_flag) printf("Timeout waiting for Echo pulse start.\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(MEASUREMENT_INTERVAL_MS));
                continue;
            }

            // --- Echo Started - Record Start Time ---
            uint64_t echoStartTime = micros_since_epoch();

            // --- Wait for Echo to End (Go Low) ---
            uint64_t timeLimitEnd = echoStartTime + POLLING_TIMEOUT_US; // Timeout limit
            while (lgGpioRead(gpio_handle, ECHO_PIN) == 1 && !shutdown_flag) {
                 if (micros_since_epoch() > timeLimitEnd) break; // Timeout waiting for end
            }

            // If timed out waiting for end, report and continue loop
            if (lgGpioRead(gpio_handle, ECHO_PIN) == 1 && !shutdown_flag) {
                 printf("Timeout waiting for Echo pulse end.\n");
                 std::this_thread::sleep_for(std::chrono::milliseconds(MEASUREMENT_INTERVAL_MS));
                 continue;
            }
            if (shutdown_flag) break; // Exit if Ctrl+C hit during wait

            // --- Echo Ended - Record End Time ---
            uint64_t echoEndTime = micros_since_epoch();

            // --- Calculate Duration and Distance ---
            uint64_t duration_us = echoEndTime - echoStartTime;

            if (duration_us > 0 && duration_us < 38000) { // Check for valid pulse width range
                double distance = static_cast<double>(duration_us) / 58.3;
                printf("Distance: %.2f cm (Pulse: %llu us)\n", distance, (unsigned long long)duration_us);
            } else {
                printf("Out of range or invalid pulse (Pulse: %llu us)\n", (unsigned long long)duration_us);
            }

            // Wait before next measurement
            std::this_thread::sleep_for(std::chrono::milliseconds(MEASUREMENT_INTERVAL_MS));

        } // End while loop

    // --- Error Catching ---
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        if (gpio_handle >= 0) {
            std::cout << "Attempting cleanup after error..." << std::endl;
            lgGpioFree(gpio_handle, ECHO_PIN);
            lgGpioFree(gpio_handle, TRIG_PIN);
            lgGpiochipClose(gpio_handle);
            gpio_handle = -1;
        }
        return 1;
    }

    // --- Normal Cleanup ---
    if (gpio_handle >= 0) {
        std::cout << "Cleaning up GPIO..." << std::endl;
        lgGpioFree(gpio_handle, ECHO_PIN);
        lgGpioFree(gpio_handle, TRIG_PIN);
        lgGpiochipClose(gpio_handle);
        std::cout << "GPIO cleaned up." << std::endl;
    }

    return 0;
}

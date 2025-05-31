// example_pulse_stopping.cpp
#include <iostream>     // For console input/output (std::cout, std::cerr)
#include <stdexcept>    // For standard exception types (std::runtime_error)
#include <chrono>       // For time durations (std::chrono::seconds, milliseconds)
#include <thread>       // For pausing execution (std::this_thread::sleep_for)
#include <csignal>      // For signal handling (SIGINT - Ctrl+C)
#include <lgpio.h>      // The C library for Raspberry Pi GPIO control
#include <string>       // For std::to_string, std::string used in error messages

// --- Constants ---
// These make the code more readable and easier to modify.
const int SERVO_PIN = 12; // GPIO pin connected to the servo's signal wire (Orange/Yellow)
const int PULSE_WIDTH_MIN = 500; // Microseconds pulse width for servo's 0-degree position
const int PULSE_WIDTH_MAX = 2400; // Microseconds pulse width for servo's 180-degree position
const int PULSE_WIDTH_MID = (PULSE_WIDTH_MIN + PULSE_WIDTH_MAX) / 2; // Calculated mid-point (~90 degrees)
const int PWM_FREQUENCY = 50;      // Standard servo frequency (50 Hz = 20ms period) [cite: 7]
const int DELAY_SECONDS = 3;       // How long to pause visually at each position in the main loop
const int MOVEMENT_WAIT_MS = 500;  // Estimated time (milliseconds) needed for the servo to physically reach its target position

// --- Global Variables ---
// 'volatile sig_atomic_t' is used for variables modified by signal handlers to ensure safe access.
volatile sig_atomic_t shutdown_flag = 0; // Flag set to 1 when Ctrl+C is pressed
int gpio_handle = -1; // Stores the handle for the opened GPIO chip, needed for lgpio functions. -1 indicates not opened yet. Used globally for cleanup.

// --- Signal Handler Function ---
// This function runs when a specific signal (like Ctrl+C) is received by the program.
void handle_signal(int signal) {
    // We only care about SIGINT (Signal Interrupt), which is sent by Ctrl+C.
    if (signal == SIGINT) {
        std::cout << "\nCtrl+C detected. Shutting down..." << std::endl;
        // Set the global flag. The main loop checks this flag to exit gracefully.
        shutdown_flag = 1;
    }
}

// --- Helper Function: setServoPosition ---
// Encapsulates the logic to move the servo and optionally stop the pulse afterwards.
// Parameters:
//   handle: The GPIO chip handle obtained from lgGpiochipOpen.
//   pin: The GPIO pin number for the servo.
//   pulse_width_us: The target pulse width in microseconds.
//   stop_pulse_after: Boolean flag - if true, stop sending pulses after moving.
void setServoPosition(int handle, int pin, int pulse_width_us, bool stop_pulse_after) {
    std::cout << "Setting pulse width to " << pulse_width_us << " us" << std::endl;
    int ret = lgTxServo(handle, pin, pulse_width_us, PWM_FREQUENCY, 0, 0);
    if (ret != LG_OKAY) {
        std::cerr << "Warning: Failed to set servo pulse width. Code: " << ret
                  << ", Error: " << lguErrorText(ret) << std::endl;
    }

    if (pulse_width_us > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(MOVEMENT_WAIT_MS));
    }

    if (stop_pulse_after && pulse_width_us > 0) {
        std::cout << "Stopping pulse..." << std::endl;
        ret = lgTxServo(handle, pin, 0, PWM_FREQUENCY, 0, 0); // Stop command
         if (ret != LG_OKAY) {
             std::cerr << "Warning: Failed to stop servo pulse. Code: " << ret
                       << ", Error: " << lguErrorText(ret) << std::endl;
         }
         // *** ADD A SMALL DELAY HERE ***
         std::this_thread::sleep_for(std::chrono::milliseconds(100)); // e.g., 10ms delay
    }
}


// --- Main Function ---
int main() {
    // --- Setup Signal Handling ---
    // Register the 'handle_signal' function to be called when SIGINT (Ctrl+C) is received.
    signal(SIGINT, handle_signal);

    // --- Main Logic Block with Error Handling ---
    try {
        // --- Initialize GPIO ---
        std::cout << "Initializing GPIO..." << std::endl;
        // Get a handle to the GPIO controller chip. Required for all other lgpio actions.
        // Tries chip 0 first (common), then chip 4 (needed on some newer Pis like Pi 5).
        gpio_handle = lgGpiochipOpen(0);
        if (gpio_handle < 0) {
            gpio_handle = lgGpiochipOpen(4);
             if (gpio_handle < 0) {
                // If both fail, throw an error with details from lguErrorText.
                throw std::runtime_error("Failed to open GPIO chip 0 or 4. Error: " + std::string(lguErrorText(gpio_handle)));
             }
             std::cout << "Opened GPIO chip 4." << std::endl;
        } else {
             std::cout << "Opened GPIO chip 0." << std::endl;
        }

        // --- Claim GPIO Pin ---
        // Reserve the specific GPIO pin (SERVO_PIN) for use by this program as an output.
        // Flags=0, Initial Level=0 (doesn't matter much for servo PWM).
        int ret = lgGpioClaimOutput(gpio_handle, 0, SERVO_PIN, 0);
        if (ret != LG_OKAY) {
            // If claiming fails (e.g., pin already in use), clean up and throw an error.
            lgGpiochipClose(gpio_handle); // Close chip before throwing
            gpio_handle = -1; // Mark as closed
            throw std::runtime_error("Failed to claim GPIO pin " + std::to_string(SERVO_PIN) + ". Error: " + std::string(lguErrorText(ret)));
        }
        std::cout << "Claimed GPIO " << SERVO_PIN << " for output." << std::endl;

        std::cout << "Starting servo sweep with Pulse Stopping. Press Ctrl+C to exit." << std::endl;

        // --- Servo Movement Loop ---
        // Runs continuously until shutdown_flag is set by the signal handler.
        while (!shutdown_flag) {
            // Move to Min Position
            setServoPosition(gpio_handle, SERVO_PIN, PULSE_WIDTH_MIN, true); // Move and STOP pulse
            std::cout << "Holding at min (pulse stopped). Waiting " << DELAY_SECONDS << "s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(DELAY_SECONDS)); // Wait
            if (shutdown_flag) break; // Check flag immediately after potentially long sleep

            // Move to Mid Position
            setServoPosition(gpio_handle, SERVO_PIN, PULSE_WIDTH_MID, true); // Move and STOP pulse
            std::cout << "Holding at mid (pulse stopped). Waiting " << DELAY_SECONDS << "s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(DELAY_SECONDS)); // Wait
            if (shutdown_flag) break;
        }

    // --- Error Catching ---
    } catch (const std::exception& e) {
        // If any std::runtime_error was thrown above, catch it here.
        std::cerr << "Error: " << e.what() << std::endl;
        // Attempt cleanup even if an error occurred during setup/loop.
        if (gpio_handle >= 0) { // Only cleanup if chip was successfully opened
             std::cout << "Attempting cleanup after error..." << std::endl;
             lgTxServo(gpio_handle, SERVO_PIN, 0, 50, 0, 0); // Try to stop pulses
             lgGpioFree(gpio_handle, SERVO_PIN);             // Try to free the pin
             lgGpiochipClose(gpio_handle);                   // Try to close the chip
        }
        return 1; // Exit program with an error code
    }

    // --- Normal Cleanup (after loop exits via Ctrl+C) ---
    if (gpio_handle >= 0) { // Check if GPIO was successfully initialized
        std::cout << "Cleaning up GPIO..." << std::endl;
        // 1. Ensure pulse is stopped: Send pulse width 0.
        lgTxServo(gpio_handle, SERVO_PIN, 0, 50, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Small delay

        // 2. Free the GPIO pin: Release the claim made by lgGpioClaimOutput.
        lgGpioFree(gpio_handle, SERVO_PIN);

        // 3. Close the GPIO chip handle: Release the connection to the GPIO controller.
        lgGpiochipClose(gpio_handle);
        std::cout << "GPIO cleaned up." << std::endl;
    }

    return 0; // Exit successfully
}
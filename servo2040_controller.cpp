#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "pico/stdlib.h"
#include "servo2040.hpp"

/*
Servo2040 Multi-Servo Controller
Converted from ESP32/PCA9685 to RP2040/Servo2040
*/

using namespace servo;

// Constants
const uint NUM_SERVOS = 18; // Servo2040 supports up to 18 servos
const int MIN_ANGLE = -140; // Minimum servo angle in degrees
const int MAX_ANGLE = 140;  // Maximum servo angle in degrees

// Track current positions in degrees
int currentPositions[NUM_SERVOS];

// Servo objects array
Servo *servos[NUM_SERVOS];

// Available servo pins on Servo2040
const uint servo_pins[NUM_SERVOS] = {
    servo2040::SERVO_1,  servo2040::SERVO_2,  servo2040::SERVO_3,  servo2040::SERVO_4,
    servo2040::SERVO_5,  servo2040::SERVO_6,  servo2040::SERVO_7,  servo2040::SERVO_8,
    servo2040::SERVO_9,  servo2040::SERVO_10, servo2040::SERVO_11, servo2040::SERVO_12,
    servo2040::SERVO_13, servo2040::SERVO_14, servo2040::SERVO_15, servo2040::SERVO_16,
    servo2040::SERVO_17, servo2040::SERVO_18
};

void setup() {
    // Initialize standard library (includes USB serial)
    stdio_init_all();
    
    // Initialize all servos following Pimoroni pattern
    for(auto s = 0u; s < NUM_SERVOS; s++) {
        servos[s] = new Servo(servo_pins[s]);
        servos[s]->init();
        
        // Set custom calibration to match your original range (-140° to +140°)
        Calibration& cal = servos[s]->calibration();
        cal.first_value((float)MIN_ANGLE);
        cal.last_value((float)MAX_ANGLE);
        
        currentPositions[s] = 0;
    }
    
    // Enable all servos (this puts them at the middle)
    for(auto s = 0u; s < NUM_SERVOS; s++) {
        servos[s]->enable();
    }
    
    printf("Servo2040 Controller initialized with %d servos\n", NUM_SERVOS);
    printf("Range: %d° to %d°\n", MIN_ANGLE, MAX_ANGLE);
    printf("Calibration: min=%.1f, max=%.1f\n", servos[0]->calibration().first_value(), servos[0]->calibration().last_value());
    printf("Ready for commands (format: ch1,pos1;ch2,pos2;...)\n");
}

void handleCommands(const char* command) {
    char* cmd_copy = (char*)malloc(strlen(command) + 1);
    strcpy(cmd_copy, command);
    
    char* token = strtok(cmd_copy, ";");
    
    while (token != NULL) {
        char* comma = strchr(token, ',');
        if (comma != NULL) {
            *comma = '\0';  // Split at comma
            int channel = atoi(token);
            int position = atoi(comma + 1);
            
            // Validate channel and position
            if (channel >= 0 && channel < (int)NUM_SERVOS && 
                position >= MIN_ANGLE && position <= MAX_ANGLE) {
                
                // Debug: print what we're about to send
                printf("Setting Ch %d to %d° (before: %.1f°)\n", 
                       channel, position, servos[channel]->value());
                
                // Move servo to position - cast to float explicitly
                // Alternative method - try this if calibration isn't working:
                // Instead of: servos[channel]->value((float)position);
                // Use direct pulse mapping:
                float pulse_us = 1500.0f + (position * 500.0f / 140.0f); // Map -140→+140 to 1000→2000µs
                servos[channel]->pulse(pulse_us);
                currentPositions[channel] = position;
                
                // Debug: print what the servo thinks it's at now
                printf("Ch %d → %4d° (actual: %.1f°)\n", 
                       channel, position, servos[channel]->value());
                       
            } else {
                printf("Invalid channel (%d) or angle (%d) out of range\n", channel, position);
            }
        }
        token = strtok(NULL, ";");
    }
    
    free(cmd_copy);
}

char* readSerialLine() {
    static char buffer[256];
    static int pos = 0;
    
    // Read all available characters at once
    while (true) {
        int c = getchar_timeout_us(0); // Non-blocking read
        
        if (c == PICO_ERROR_TIMEOUT) {
            break; // No more data available
        }
        
        if (c == '\n' || c == '\r') {
            if (pos > 0) {
                buffer[pos] = '\0';
                pos = 0;
                return buffer;
            }
        } else if (c >= 32 && c <= 126 && pos < 255) { // Printable characters only
            buffer[pos++] = c;
        }
    }
    
    return NULL; // No complete line yet
}

int main() {
    setup();
    
    while (true) {
        // Tight loop for maximum responsiveness
        bool had_input = false;
        
        // Process all available input without delays
        for (int i = 0; i < 100; i++) { // Check up to 100 times per cycle
            char* command = readSerialLine();
            if (command != NULL) {
                handleCommands(command);
                had_input = true;
            } else {
                break; // No more input available
            }
        }
        
        // Only sleep if we had no input this cycle
        if (!had_input) {
            sleep_ms(1);
        }
    }
    
    // Cleanup (this won't be reached in normal operation)
    for(auto s = 0u; s < NUM_SERVOS; s++) {
        servos[s]->disable();
        delete servos[s];
    }
    
    return 0;
}
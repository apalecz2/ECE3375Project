#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
	
#define LED_BASE 0xFF200000
#define KEY_BASE 0xFF200050
#define SW_BASE 0xFF200040
#define TIMER_BASE 0xFFFEC600

// Timer register offsets
volatile int *TIMER_LOAD    = (int *)(TIMER_BASE + 0x00);
volatile int *TIMER_CONTROL = (int *)(TIMER_BASE + 0x08);
volatile int *TIMER_STATUS  = (int *)(TIMER_BASE + 0x0C); 

// GPIO addresses
volatile int *LED_PTR = (int *)LED_BASE;
volatile int *BUTTON_PTR = (int *)KEY_BASE;
volatile int *SWITCH_PTR = (int *)SW_BASE;

// HEX display
#define HEX3_HEX0_BASE ((volatile uint32_t *)0xFF200020)
uint8_t hex_digit_encoding[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

// System state variables
bool motion_detected = false;
bool audio_alert_enabled = false;
int sensitivity_level = 5;
int prev_button_state = 0;
unsigned long last_button_press_time = 0; // Track last press time
unsigned long button_cooldown = 60000000; // Cooldown period in microseconds

// Timer control functions
void start_timer() {
    stop_timer();
    *TIMER_LOAD = 2000000000; // 10 seconds
    *TIMER_STATUS = 1;        // Clear it **before** enabling
    *TIMER_CONTROL = 0b011;
}

void stop_timer() {
    *TIMER_CONTROL = 0;
}

unsigned long get_elapsed_time() {
    // You can replace this with actual time fetching logic depending on your system
    static unsigned long counter = 0;
    counter++;
    return counter * 1000;  // Simulate elapsed time in microseconds (1 counter unit = 1 microsecond)
}

void button_ISR() {
    unsigned long current_time = get_elapsed_time();

    // Check if cooldown period has passed
    if ((current_time - last_button_press_time) >= button_cooldown) {
        int button_state = *BUTTON_PTR;

        // If button is pressed (held down or newly pressed)
        if (button_state & 0x1) { // Button 1 (Increase sensitivity)
            sensitivity_level = (sensitivity_level < 10) ? sensitivity_level + 1 : 10;
            printf("Sensitivity increased: %d\n", sensitivity_level);
        } else if (button_state & 0x2) { // Button 2 (Decrease sensitivity)
            sensitivity_level = (sensitivity_level > 1) ? sensitivity_level - 1 : 1;
            printf("Sensitivity decreased: %d\n", sensitivity_level);
        } else if (button_state & 0x4) { // Button 3 (Trigger motion)
            static int press_count = 0;
            press_count++;
            if (press_count >= (11 - sensitivity_level)){
                printf("Motion detected...\n");
                motion_sensor_ISR();
                press_count = 0;
            } else {
                printf("Motion press count: %d\n", press_count);
            }
        }

        last_button_press_time = current_time; // Update the last button press time
    }
}

void switch_ISR() {
    static int last_switch_state = 0;
    int switch_state = *SWITCH_PTR;
    if (switch_state != last_switch_state) {
        audio_alert_enabled = switch_state & 0x1;
        printf("Audio Alert %s\n", audio_alert_enabled ? "Enabled" : "Disabled");
    }
    last_switch_state = switch_state;
}

void motion_sensor_ISR() {
    *LED_PTR |= 0x1;  // Turn on LED (simulate relay activating light)
    start_timer();
    motion_detected = true;
    if (audio_alert_enabled) {
        printf("Audio Alert: Motion Detected!\n");
    }
}

void timer_ISR() {
    stop_timer();
    *TIMER_STATUS = 1;
    *LED_PTR &= ~0x1;  // Turn off LED bit 0 (simulate light turning off)
    motion_detected = false;
    printf("Light Turned Off\n");
}

// Main function
int main() {
    printf("Initializing System...\n");

    stop_timer();
    *TIMER_STATUS = 1; // clear any false startup flag
    *LED_PTR = 0; // LED is off

    while (1) {
        int button_state = *BUTTON_PTR;
        if (button_state) button_ISR(); // Only call ISR if button is pressed

        int switch_state = *SWITCH_PTR;
        if (switch_state) switch_ISR();

        if (motion_detected && (*TIMER_STATUS & 0x1)) {
            *TIMER_STATUS = 1; // clear timeout flag
            timer_ISR();
        }
        
        // Put sensitivity on hex
        uint32_t hex_values_seconds_hundredths = (hex_digit_encoding[0 / 10] << 24) |
                                             (hex_digit_encoding[0 % 10] << 16) |
                                             (hex_digit_encoding[sensitivity_level / 10] << 8) |
                                             (hex_digit_encoding[sensitivity_level % 10]);
        *HEX3_HEX0_BASE = hex_values_seconds_hundredths; // seconds and hundredths in HEX3-HEX0
        
        
    }

    return 0;
}

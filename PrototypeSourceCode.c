/*
    The first and second buttons turn up and down sensitivity.
    The third button mocks motion being detected. Based on sensitivity level, the button has to be pressed more times for motion to be detected.
    
    If motion is detected, the led will turn on.
    The led will represent the status of the light being on or off.
    
    The volume control is toggled with the first switch.
    
    The light will be turned off after some time.
    
    The user preferences for sensitivity etc, will be stored in variables to mock reading from a stored state to save preferences.
    

*/


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
	
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

// System state variables
bool motion_detected = false;
bool audio_alert_enabled = false;
int sensitivity_level = 5;
int prev_button_state = 0;

void start_timer() {
    stop_timer();
    *TIMER_LOAD = 2000000000; // 10 seconds
    *TIMER_STATUS = 1;        // Clear it **before** enabling
    *TIMER_CONTROL = 0b011;
}
void stop_timer(){
    *TIMER_CONTROL = 0;
}

// Interrupt Service Routines (ISRs)
void motion_sensor_ISR() {
    *LED_PTR |= 0x1;  // Turn on LED (simulate relay activating light)
    start_timer();
    motion_detected = true;
    if (audio_alert_enabled) {
        printf("Audio Alert: Motion Detected!\n");
    }
    // timer_counter = 10;
    // *TIMER_PTR = 5000000; // Set delay timer for 5 seconds
}

void button_ISR() {
    int button_state = *BUTTON_PTR;
    int changed = button_state & ~prev_button_state; // detects rising edge
    prev_button_state = button_state;

    if (changed & 0x1) {
        sensitivity_level++;
        if (sensitivity_level > 10) sensitivity_level = 10;
        printf("Sensitivity increased: %d\n", sensitivity_level);
        delay(100000);  // debounce
    } else if (changed & 0x2) {
        sensitivity_level--;
        if (sensitivity_level < 1) sensitivity_level = 1;
        printf("Sensitivity decreased: %d\n", sensitivity_level);
        delay(100000);  // debounce
    } else if (changed & 0x4) {
        // TODO: add this
        static int press_count = 0;
        press_count++;
        // the higher the sensitivity level, the fewer presses needed to trigger motion
        // i.e. sensitivity is high (10), only requires 1 button press
        // i.e. sensitivity is low (1), requires 10 buttons presses
        if (press_count >= (11 - sensitivity_level)){
            printf("Motion detected...");
            motion_sensor_ISR();
            press_count = 0;
        } else {
            printf("Motion press count: %d\n", press_count);
        }
        delay(100000);  // debounce
    }
}

void switch_ISR() {

    static int last_switch_state = 0;
    int switch_state = *SWITCH_PTR;
    if (switch_state != last_switch_state){
        audio_alert_enabled = switch_state & 0x1;
        printf("Audio Alert %s\n", audio_alert_enabled ? "Enabled" : "Disabled");
    }
    last_switch_state = switch_state;
}

void timer_ISR() {
    stop_timer();
    *TIMER_STATUS = 1;
    *LED_PTR &= ~0x1;  // Turn off LED bit 0 (simulate light turning off)
    motion_detected = false;
    printf("Light Turned Off\n");
}

void delay(int cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++); // Simple loop to create a delay
}


// TODO: status led for debugging
// void update_status_led(){
//     if (motion_detected) {
//         *LED_PTR |= 0x2; // turn on bit 1 without affecting other bits
//     } else {
//         *LED_PTR &= ~0x2; // turn off bit 1 without affecting other bits
//     }
// }

// Main function
int main() {
    printf("Initializing System...\n");

    stop_timer();
    *TIMER_STATUS = 1; // clear any false startup flag
    *LED_PTR = 0; // LED is off

    while (1) {
        if (*BUTTON_PTR) button_ISR();
        if (*SWITCH_PTR) switch_ISR();

        // check if timer expired
        if (motion_detected && (*TIMER_STATUS & 0x1)) {
            *TIMER_STATUS = 1; // clear timeout flag
            timer_ISR();
        }
    }
    return 0;
}

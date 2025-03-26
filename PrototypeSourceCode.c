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
	
#define LED_BASE	      0xFF200000
#define KEY_BASE              0xFF200050
#define SW_BASE               0xFF200040
#define TIMER_BASE            0xFF202000

// GPIO addresses
volatile int *LED_PTR = (int *)LED_BASE;
volatile int *BUTTON_PTR = (int *)KEY_BASE;
volatile int *SWITCH_PTR = (int *)SW_BASE;
volatile int *TIMER_PTR = (int *)TIMER_BASE;

// System state variables
bool motion_detected = false;
bool audio_alert_enabled = false;
int sensitivity_level = 5;
int timer_counter = 0;

// Interrupt Service Routines (ISRs)
void motion_sensor_ISR() {
    motion_detected = true;
    *LED_PTR = 0x1;  // Turn on LED (simulate relay activating light)
    *TIMER_PTR = 10; // 5 secs?
    if (audio_alert_enabled) {
        printf("Audio Alert: Motion Detected!\n");
    }
    // timer_counter = 10;
    // *TIMER_PTR = 5000000; // Set delay timer for 5 seconds
}

void button_ISR() {
    int button_state = *BUTTON_PTR;
    if (button_state & 0x1) {
        sensitivity_level++;
        if (sensitivity_level > 10) sensitivity_level = 10;
        printf("Sensitivity increased: %d\n", sensitivity_level);
    } else if (button_state & 0x2) {
        sensitivity_level--;
        if (sensitivity_level < 1) sensitivity_level = 1;
        printf("Sensitivity decreased: %d\n", sensitivity_level);
    } else if (button_state & 0x4) {
        // TODO: add this
        static int press_count = 0;
        press_count++;
        // the higher the sensitivity level, the fewer presses needed to trigger motion
        // i.e. sensitivity is high (10), only requires 1 button press
        // i.e. sensitivity is low (1), requires 10 buttons presses
        if (press_count >= (11 - sensitivity_level)){
            motion_sensor_ISR();
            press_count = 0;
        } else {
            printf("Motion press count: %d\n", press_count)
        }
    }
    printf("Sensitivity Level: %d\n", sensitivity_level);
}

void switch_ISR() {
    audio_alert_enabled = *SWITCH_PTR & 0x1;
    printf("Audio Alert %s\n", audio_alert_enabled ? "Enabled" : "Disabled");
}

void timer_ISR() {
    if (!motion_detected) {
        *LED_PTR &= ~0x1;  // Turn off LED bit 0 (simulate light turning off)
        printf("Light Turned Off\n");
    }
    motion_detected = false;
}
void delay(int cycles) {
    volatile int i;
    for (i = 0; i < cycles; i++); // Simple loop to create a delay
}

// Main function
int main() {
    printf("Initializing System...\n");
    while (1) {
        if (*BUTTON_PTR) button_ISR();
        if (*SWITCH_PTR) switch_ISR();

        if (timer_counter > 0){
            timer_counter--;
            if (timer_counter == 0){
                timer_ISR();
            }
        }

        //update status LED
        update_status_led();

        delay(500000); // Simulate delay
    }
    return 0;
}



// TODO: the timer needs to go down - Harrison finished this, just needs to be tested.

// TODO: status led
void update_status_led(){
    if (motion_detected) {
        *LED_PTR |= 0x2; // turn on bit 1 without affecting other bits
    } else {
        *LED_PTR &= ~0x2; // turn off bit 1 without affecting other bits
    }
}

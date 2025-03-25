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

// Interrupt Service Routines (ISRs)
void motion_sensor_ISR() {
    motion_detected = true;
    *LED_PTR = 0x1;  // Turn on LED (simulate relay activating light)
    if (audio_alert_enabled) {
        printf("Audio Alert: Motion Detected!\n");
    }
    *TIMER_PTR = 5000000; // Set delay timer for 5 seconds
}

void button_ISR() {
    int button_state = *BUTTON_PTR;
    if (button_state & 0x1) {
        sensitivity_level++;
        if (sensitivity_level > 10) sensitivity_level = 10;
    } else if (button_state & 0x2) {
        sensitivity_level--;
        if (sensitivity_level < 1) sensitivity_level = 1;
    } else if (button_state & 0x3) {
        // TODO: add this
    }
    printf("Sensitivity Level: %d\n", sensitivity_level);
}

void switch_ISR() {
    audio_alert_enabled = *SWITCH_PTR & 0x1;
    printf("Audio Alert %s\n", audio_alert_enabled ? "Enabled" : "Disabled");
}

void timer_ISR() {
    if (!motion_detected) {
        *LED_PTR = 0x0;  // Turn off LED (simulate light turning off)
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
        if (*TIMER_PTR == 0) timer_ISR();
        delay(500000); // Simulate delay
    }
    return 0;
}



// TODO: the timer needs to go down
// TODO: status led
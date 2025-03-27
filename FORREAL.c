#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#define LED_BASE 0xFF200000
#define KEY_BASE 0xFF200050
#define SW_BASE 0xFF200040
#define TIMER_BASE 0xFFFEC600

// Timer register offsets
volatile int *TIMER_LOAD = (int *)(TIMER_BASE + 0x00);
volatile int *TIMER_CONTROL = (int *)(TIMER_BASE + 0x08);
volatile int *TIMER_STATUS = (int *)(TIMER_BASE + 0x0C);

// GPIO addresses
volatile int *LED_PTR = (int *)LED_BASE;
volatile int *BUTTON_PTR = (int *)KEY_BASE;
volatile int *SWITCH_PTR = (int *)SW_BASE;

// HEX display
// Define base address for HEX3-HEX0 (controls rightmost four displays)
#define HEX3_HEX0_BASE ((volatile uint32_t *)0xFF200020)
uint8_t hex_digit_encoding[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

// Update only HEX1 and HEX0 (rightmost two)
void update_hex_display(int sensitivity_level)
{
    uint32_t hex_values = (hex_digit_encoding[sensitivity_level / 10] << 8) |
                          (hex_digit_encoding[sensitivity_level % 10]);

    // Only affects HEX1-HEX0
    *HEX3_HEX0_BASE = hex_values; 
}

// SHow the timer on the hexxx
#define HEX5_HEX4_BASE ((volatile uint32_t *)0xFF200030)
volatile int *TIMER_VALUE = (int *)(TIMER_BASE + 0x04);
void update_timer_display()
{
    // Convert clock cycles to seconds
    int remaining_time = (*TIMER_VALUE / 200000000) + 1; 
    if (remaining_time > 10)
        remaining_time = 10;
    
    uint32_t hex_values = (hex_digit_encoding[remaining_time / 10] << 8) |
                          (hex_digit_encoding[remaining_time % 10]);
    // Only updates HEX5-HEX4
    *HEX5_HEX4_BASE = hex_values; 
}

// System state variables
bool motion_detected = false;
bool audio_alert_enabled = false;
int sensitivity_level = 5;
int prev_button_state = 0;
// Track last press time
unsigned long last_button_press_time = 0; 
// Cooldown period in microseconds
unsigned long button_cooldown = 60000000; 

// Timer control functions
void stop_timer()
{
    *TIMER_CONTROL = 0;
}

void start_timer()
{
    stop_timer();
    // 10 seconds
    *TIMER_LOAD = 2000000000; 
    *TIMER_STATUS = 1;  
    // Clear it **before** enabling      
    *TIMER_CONTROL = 0b011;
}

unsigned long get_elapsed_time()
{
    static unsigned long counter = 0;
    counter++;
    // Simulate elapsed time in microseconds (1 counter unit = 1 microsecond)
    return counter * 1000; 
}

void motion_sensor_ISR()
{
    // Turn on LED (simulate relay activating light)
    *LED_PTR |= 0x1; 
    start_timer();
    motion_detected = true;
    if (audio_alert_enabled)
    {
        printf("Audio Alert: Motion Detected!\n");
    }
}

void button_ISR()
{
    unsigned long current_time = get_elapsed_time();

    // Check if cooldown period has passed
    if ((current_time - last_button_press_time) >= button_cooldown)
    {
        int button_state = *BUTTON_PTR;

        // If button is pressed (held down or newly pressed)
        if (button_state & 0x1)
        { 
            // Button 1 (Increase sensitivity)
            sensitivity_level = (sensitivity_level < 10) ? sensitivity_level + 1 : 10;
            printf("Sensitivity increased: %d\n", sensitivity_level);
        }
        else if (button_state & 0x2)
        { 
            // Button 2 (Decrease sensitivity)
            sensitivity_level = (sensitivity_level > 1) ? sensitivity_level - 1 : 1;
            printf("Sensitivity decreased: %d\n", sensitivity_level);
        }
        else if (button_state & 0x4)
        { 
            // Button 3 (Trigger motion)
            static int press_count = 0;
            press_count++;
            if (press_count >= (11 - sensitivity_level))
            {
                printf("Motion detected...\n");
                motion_sensor_ISR();
                press_count = 0;
            }
            else
            {
                printf("Motion press count: %d\n", press_count);
            }
        }

        // Update the last button press time
        last_button_press_time = current_time; 
    }
}

void switch_ISR()
{
    static int last_switch_state = 0;
    int switch_state = *SWITCH_PTR;
    if (switch_state != last_switch_state)
    {
        audio_alert_enabled = switch_state & 0x1;
        printf("Audio Alert %s\n", audio_alert_enabled ? "Enabled" : "Disabled");
    }
    last_switch_state = switch_state;
}

void timer_ISR()
{
    stop_timer();
    *TIMER_STATUS = 1;
    // Turn off LED bit 0 (simulate light turning off)
    *LED_PTR &= ~0x1; 
    motion_detected = false;
    printf("Light Turned Off\n");
}

// Main function
int main()
{
    printf("Starting System...\n");

    stop_timer();
    *TIMER_LOAD = 2000000000;
    // clear any false startup flag
    *TIMER_STATUS = 1; 
    // LED is off
    *LED_PTR = 0;      

    while (1)
    {
        int button_state = *BUTTON_PTR;
        if (button_state)
            // Only call ISR if button is pressed
            button_ISR(); 

        int switch_state = *SWITCH_PTR;
        if (switch_state)
            switch_ISR();

        if (motion_detected && (*TIMER_STATUS & 0x1))
        {
            // clear timeout flag
            *TIMER_STATUS = 1; 
            timer_ISR();
        }

        // To help debug we will show the sensitivity on the hex display
        update_hex_display(sensitivity_level);

        // also show the timer for debuging (Seconds)
        update_timer_display();
    }

    return 0;
}

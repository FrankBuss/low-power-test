// Simple test program for testing sleep mode current with data retention.
// blinks the LED in a loop x times with 0.2 s delay, then 0.5 s delay,
// and then starts again. After button push, goes to sleep.
// Another button push wakes up and increments x, then blinks the LED etc.

// PIC10F202 version - assumes 4MHz internal oscillator
// GP0 = LED output
// GP3 = Button input with external pull-up (GP3 is input only)

#define _XTAL_FREQ 4000000   // 4MHz internal oscillator
#include <xc.h>

// Configuration bits
#pragma config WDTE = OFF    // Watchdog Timer disabled
#pragma config CP = OFF      // Code protection off
#pragma config MCLRE = OFF   // GP3/MCLR pin function is GP3

// Global variable to test RAM retention across sleep
// 'persistent' prevents C startup code from clearing it on reset/wake
__persistent uint32_t blink_count;

void main(void) {
    // Configure I/O for lowest power consumption
    TRIS = 0b00001000;  // GP3 input (button), GP0,1,2 outputs
    GPIO = 0;            // All outputs low initially (including unused GP1, GP2)

    // Check if we're waking from sleep (STATUS bit 7 = GPWUF)
    if (STATUSbits.GPWUF) {
        // We woke from sleep - increment counter
        blink_count++;

        // Wait for button to be pressed (it triggered the wake)
        while (GP3);    // Wait while button not pressed
        __delay_ms(50); // debounce

        // Wait for button release
        while (!GP3);   // Wait for release
        __delay_ms(50); // debounce
    } else {
        // Normal reset (power-on or MCLR) - initialize counter
        blink_count = 1;
    }

    // Configure OPTION register - no wake-on-change yet
    OPTION = 0b11000000; // Bit 7=1 (GPWU disabled), Bit 6=1 (GPPU disabled)

    while (1) {
        // Blink pattern: blink N times (0.2s on, 0.2s off), then 0.5s pause
        for (uint32_t i = 0; i < blink_count; i++) {
            GP0 = 1;  // LED on

            // Check button during LED on phase (200ms in 10ms chunks)
            for (unsigned char j = 0; j < 20; j++) {
                __delay_ms(10);
                if (!GP3) goto button_pressed;  // GP3 low = button pressed
            }

            GP0 = 0;  // LED off

            // Check button during LED off phase (200ms in 10ms chunks)
            for (unsigned char j = 0; j < 20; j++) {
                __delay_ms(10);
                if (!GP3) goto button_pressed;
            }
        }

        // 0.5 second pause after blink sequence
        for (unsigned char j = 0; j < 50; j++) {
            __delay_ms(10);
            if (!GP3) goto button_pressed;
        }
        continue;

button_pressed:
        // Button was pressed - wait for release
        GP0 = 0;  // Make sure LED is off
        __delay_ms(50);  // 50ms debounce
        while (!GP3);   // Wait for button release
        __delay_ms(50);  // 50ms debounce

        // Enable wake-on-pin-change and enter sleep
        OPTION = 0b01000000; // Bit 7=0 (GPWU enabled)
        SLEEP();            // Enter sleep mode - will RESET on wake!
        NOP();              // Never reached - PIC resets on wake
    }
}

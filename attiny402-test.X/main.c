// Simple test program for testing sleep mode current with data retention.
// blinks the LED in a loop x times with 0.5 s delay, then 1 s delay,
// and then starts again. After button push, goes to sleep.
// Another button push wakes up and increments x, then blinks the LED etc.

// flashed with serialupdi with a FTDI serial port adapter, and a schottky diode:
// https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md
// avrdude -c serialupdi -p t402 -P /dev/ttyUSB0 -U flash:w:/home/frank/data/projects/low-power-test/attiny402-test.X/dist/default/production/attiny402-test.X.production.hex:i -F

#define F_CPU 3333333UL // Default internal oscillator is 20MHz/6 = 3.33MHz
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Global variable to test RAM retention across sleep
uint32_t blink_count = 1;  // Start with 1 blink

ISR(PORTA_PORT_vect){
	PORTA.INTFLAGS = PIN6_bm;  // Clear interrupt flag for PA6
}

int main(void){
	// Disable WDT
	_PROTECTED_WRITE(WDT.CTRLA, 0x00);

	// Set unused pins as INPUT_PULLUP to prevent floating
	PORTA.PIN0CTRL = PORT_PULLUPEN_bm;  // PA0
	PORTA.PIN1CTRL = PORT_PULLUPEN_bm;  // PA1
	PORTA.PIN3CTRL = PORT_PULLUPEN_bm;  // PA3
	PORTA.PIN7CTRL = PORT_PULLUPEN_bm;  // PA7

	// LED on PA2
	PORTA.DIRSET = PIN2_bm;

	// Button on PA6 with external pull-up, interrupt on falling edge
	PORTA.PIN6CTRL = PORT_ISC_FALLING_gc;

	// Configure sleep mode for lowest power
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sei();  // Enable interrupts

	while (1) {
		// Blink pattern: blink N times (0.2s on, 0.2s off), then 0.5s pause
		for (uint32_t i = 0; i < blink_count; i++) {
			PORTA.OUTSET = PIN2_bm;  // LED on

			// Check button during LED on phase (200ms in 10ms chunks)
			for (uint8_t j = 0; j < 20; j++) {
				_delay_ms(10);
				if (!(PORTA.IN & PIN6_bm)) goto button_pressed;
			}

			PORTA.OUTCLR = PIN2_bm;  // LED off

			// Check button during LED off phase (200ms in 10ms chunks)
			for (uint8_t j = 0; j < 20; j++) {
				_delay_ms(10);
				if (!(PORTA.IN & PIN6_bm)) goto button_pressed;
			}
		}

		// 0.5 second pause after blink sequence
		for (uint8_t j = 0; j < 50; j++) {
			_delay_ms(10);
			if (!(PORTA.IN & PIN6_bm)) goto button_pressed;
		}
		continue;

button_pressed:
		// Button was pressed - wait for release
		PORTA.OUTCLR = PIN2_bm;  // Make sure LED is off
		_delay_ms(50);  // Simple debounce
		while (!(PORTA.IN & PIN6_bm));
		_delay_ms(50);

		// Enter sleep
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		// Woken up by button press - increment blink count
		blink_count++;

		// Wait for button release after wake
		_delay_ms(50);
		while (!(PORTA.IN & PIN6_bm));
		_delay_ms(50);
	}
}

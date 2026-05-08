#include <avr/io.h>
#define F_CPU 8000000UL  // 8MHz
#include <stdint.h>
#include <util/delay.h>

/*
 * FILE: lab13-2.c
 *
 * Replace the "<-YOUR CODE HERE->" lines with your code.
 *
 * Push buttons B0 to B3 are connected to port C, pins 0 to 3.
 * Button B0 (pin C0) increases the frequency
 * Button B1 (pin C1) decreases the frequency
 * Button B2 (pin C2) increases the duty cycle
 * Button B3 (pin C3) decreases the duty cycle
 *
 * A piezo buzzer and an LED should both be connected to the OC1B pin
 * (port D, pin 4). (The other end of the piezo buzzer should be connected
 * to ground (0V).)
 */

// For a given frequency (Hz), return the clock period (in terms of the
// number of clock cycles of a 1MHz clock)
uint16_t freq_to_clock_period(uint16_t freq) {
    return (1000000UL / freq);  // UL makes the constant an unsigned long (32 bits)
                                // and ensures we do 32 bit arithmetic, not 16
}

// Return the width of a pulse (in clock cycles) given a duty cycle (%) and
// the period of the clock (measured in clock cycles)
uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod) {
    return (dutycycle * clockperiod) / 100;
}

int pwm() {
    uint16_t freq = 200;  // Hz
    float dutycycle = 2;  // %
    uint16_t clockperiod = freq_to_clock_period(freq);
    uint16_t pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);

    // Make pin OC1B be an output
    DDRD |= (1 << DDD4);  // ddd4 is the pin for oc1b, so we set it to 1 to make it an output

    // Set the maximum count value for timer/counter 1 to be one less than the clockperiod
    OCR1A = clockperiod - 1;

    // Set the count compare value based on the pulse width. The value will be 1 less
    // than the pulse width - unless the pulse width is 0.
    if (pulsewidth == 0) {
        OCR1B = 0;
    } else {
        OCR1B = pulsewidth - 1;
    }

    // Set up timer/counter 1 for Fast PWM, counting from 0 to the value in OCR1A
    // before reseting to 0. Count at 1MHz (CLK/8).
    // Configure output OC1B to be clear on compare match and set on timer/counter
    // overflow (non-inverting mode).
    TCCR1A = (1 << COM1B1) | (1 << WGM11) |
             (1 << WGM10);                // non-inverting mode, fast pwm with top in OCR1A
    TCCR1B = (1 << WGM12) | (1 << CS11);  // fast pwm with top in OCR1A, clk/8 prescaler

    // PWM output should now be happening - at the frequency and pulse width set above

    while (1) {
        // Check the state of the buttons (on port C) every 100ms.
        _delay_ms(100);

        if (PINC & (1 << PINC0)) {  // increase frequency by 5%, but highest frequency is 10000Hz
            freq = freq * 105UL / 100UL;  // Constants made 32 bit to ensure 32 bit arithmetic
            if (freq > 10000) {
                freq = 10000;
            }
        }
        if (PINC & (1 << PINC1)) {       // decrease frequency by 5%, but lowest frequency is 20Hz
            freq = freq * 95UL / 100UL;  // Constants made 32 bits to ensure 32 bit arithmetic
            if (freq < 20) {
                freq = 20;
            }
        }
        if (PINC &
            (1 << PINC2)) {  // increase duty cycle by 0.1 if less than 10% or 1 if 10% or higher
            if (dutycycle < 10) {
                dutycycle += 0.1;
            } else {
                dutycycle += 1.0;
                if (dutycycle > 100) {
                    dutycycle = 100;
                }
            }
        }
        if (PINC &
            (1 << PINC3)) {  // decrease duty cycle by 0.1 if less than 10% or 1 if 10% or higher
            if (dutycycle < 10) {
                dutycycle -= 0.1;
                if (dutycycle < 0) {
                    dutycycle = 0;
                }
            } else {
                dutycycle -= 1.0;
            }
        }

        // Work out the clock period and pulse width
        clockperiod = freq_to_clock_period(freq);
        pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);

        // Update the PWM registers
        if (pulsewidth > 0) {
            // The compare value is one less than the number of clock cycles in the pulse width
            OCR1B = pulsewidth - 1;
        } else {
            OCR1B = 0;
        }
        // Note that a compare value of 0 results in special behaviour - see page 130 of the
        // datasheet (2018 version)

        // Set the maximum count value for timer/counter 1 to be one less than the clockperiod
        OCR1A = clockperiod - 1;
    }
}

// task 1

/*
 * FILE: lab13-1.c
 *
 * Replace the "<-YOUR CODE HERE->" lines with your code.
 *
 * LEDs are connected to pins B3 (OC0A) and B4 (OC0B)
 */

// task 1: fade the LED on pin B3 from fully on to fully off and back to fully on, with a period of
// about 2 seconds. The LED on pin B4 should do the opposite (i.e. when the LED on pin B3 is fully
// on, the LED on pin B4 should be fully off, and vice versa).
int task_1() {
    // Make bits 3 and 4 of port B outputs (OC0A and OC0B pins)
    DDRB = (1 << DDB3) | (1 << DDB4);

    // Set up output compare value A to be 0 and output compare value B to be 255
    OCR0A = 0;
    OCR0B = 255;

    // Set up timer/counter 0 for Fast PWM, with both pins OC0A and OC0B
    // to be SET on compare match (i.e. inverting mode) and cleared on
    // timer/counter overflow.
    // Set the timer/counter to count at 8MHz (no clock scaling).
    // (See pages 109 to 113 of the datasheet for details)
    TCCR0A =
        (1 << COM0A1) | (1 << COM0A0) | (1 << COM0B1) | (1 << COM0B0) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS02) | (1 << CS00);

    // Delay one second before we start the fading
    _delay_ms(1000);

    while (1) {
        while (OCR0A < 255) {
            // count up - LED on A should decrease in brightness
            // LED on B should increase in brightness
            OCR0A++;
            OCR0B = 255 - OCR0A;
            _delay_ms(5);
        }
        while (OCR0A > 0) {
            // count down - LED on A should increase in brightness
            // LED on B should decrease in brightness
            OCR0A--;
            OCR0B = 255 - OCR0A;
            _delay_ms(5);
        }
    }

    // After you try the code on the board, try a clock prescaler of CLK/1024.
    // How do you explain the behaviour of the LEDs?
}

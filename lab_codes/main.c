/*
 * lab16-1.c
 *
 * Joystick ADC example
 *
 * Replace the "<-YOUR CODE HERE->" comments with your code.
 */

#include <avr/interrupt.h>
#include <avr/io.h>

// connect the joystick X-axis (L/R) output to pin A0 and the Y-axis (U/D) output to pin A1
// connect the seven-segment display to port C (SSD-A to pin C0 through SSD-DP to pin C7)
// connect the seven-segment common cathode to pin D7
// connect the LEDs to port B (L0 to pin B0 through L7 to pin B7)

// this code will output a representation of the value of `x` to the right digit of the SSD
// and a representation of the value of `y` to the left
// it will scale these values from the range 0-1023 inclusive to the range 0-19 inclusive
// values in the range 10-19 will be displayed as letters A-J (as well as can be done on the SSD)
// `x` will also be shown in binary on the LEDs, scaled from 0-1023 to 0-255
uint16_t x, y;
const uint8_t figures[20] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67,   // 0 - 9
                             0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x30, 0x1E};  // A - J
uint8_t digit;

int main(void) {
    // SET UP THE JOYSTICK
    // `axis` will be 0 to read the X-axis of
    // the joystick, or 1 to read the Y-axis
    uint8_t axis = 1;
    // set the ADC to use AVCC as the reference voltage
    // leave the ADC as right-adjust
    // no need to touch the MUX bits for now - they'll be updated below as we choose which axis to
    // read from
    ADMUX = (1 << REFS0);
    // enable the ADC, and use a clock divisor to get between 50kHz and 200kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // SET UP THE OUTPUT
    // set all eight bits of port B to be outputs
    DDRB = 0xFF;
    // set all eight bits of port C to be outputs
    DDRC = 0xFF;
    // set pin D7 to be an output
    DDRD = (1 << DDD7);
    // `digit` will be 0 to display the right (X-axis) reading
    // and 1 to display the left (Y-axis) reading
    digit = 0;
    // set up an interrupt to run every 10 ms
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A = 78;
    TIMSK2 = 1 << OCIE2A;
    sei();

    while (1) {
        // if `axis` is 0, set the MUX[4:0] bits of ADMUX to read from ADC0 = pin A0 = X-axis
        // otherwise, if `axis` is 1, set the MUX[4:0] bits to read from ADC1 = pin A1 = Y-axis
        // do NOT change the REFS1, REFS0, or ADLAR bits - use bitmasking
        /* <YOUR CODE HERE> - there are several ways of doing this, and you'll probably need
         * multiple lines*/
        // start the ADC conversion
        ADCSRA |= (1 << ADSC);
        // while the conversion is ongoing, do nothing
        // when the AVR completes the conversion, it will use its
        // internal hardware to automatically clear the bit
        while (ADCSRA & (1 << ADSC)) {
        }

        // if `axis` is 1, we just read the Y-axis, so allocate the ADC result to `y`
        // otherwise, we just read the X-axis, so allocate the result to `x`
        if (axis) {
            y = ADC;
        } else {
            x = ADC;
            // also output the result to the LEDs (scaled down to 8-bit)
            PORTB = (x >> 2) & 0xFF;
        }
        // toggle `axis`, so that the next loop will read the other joystick axis
        axis = axis ? 0 : 1;
    }
}

// runs every 10ms
ISR(TIMER2_COMPA_vect) {
    // if `digit` is 1, output the value of `y`, scaled and encoded, to the left display of the SSD
    // otherwise, if `digit` is 0, output `x`, scaled and encoded, to the right
    if (digit) {
        PORTC = figures[(20 * y) >> 10];
        PORTD |= 1 << 7;
    } else {
        PORTC = figures[(20 * x) >> 10];
        PORTD &= ~(1 << 7);
    }
    // toggle `digit` so that the next interrupt will output to the other display
    // NB: this is a different method to toggle than the one used for `axis` above
    digit = !digit;
    // there are many other methods e.g. assigning a constant in each branch of the
    // `if` above, subtracting the current value from 1, XORing with 1, assigning it the
    // result of an equal-to-zero check, indexing the world's shortest lookup table, etc.
}
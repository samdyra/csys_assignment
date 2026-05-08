/*
** Lab 14-1 - Interrupts - Stopwatch
**
** IO Board connections
**   Seven segment display A to DP connected to AVR port A
**   Seven segment display CC connected to AVR port C, pin 0
**   Button B0 connected to AVR port D, pin 2 - this is the start/stop button
**   Button B1 connected to AVR port D, pin 3 - this is the reset button
*/

#include <avr/interrupt.h>
#include <avr/io.h>

/* The following macros can be used to check for buttons being pushed.
** STARTSTOP will be 1 if the port D pin 2 button is down, 0 otherwise.
** RESET will be 1 if the port D pin 3 button is down, 0 otherwise.
*/
#define STARTSTOP ((PIND & (1 << 2)) >> 2)
#define RESET ((PIND & (1 << 3)) >> 3)

/* Control variables */

/* stopwatch_timing - 1 if the stopwatch is running, 0 if not.
** Stopwatch is initially stopped.
*/
volatile uint8_t stopwatch_timing = 0;

/* digits_displayed - 1 if digits are displayed on the seven
** segment display, 0 if not. No digits displayed initially.
*/
volatile uint8_t digits_displayed = 0;

/* Time value - we count hundredths of seconds,
** i.e. increment the count every 10ms.
*/
volatile uint16_t count = 0;

/* Seven segment display digit being displayed.
** 0 = right digit; 1 = left digit.
*/
volatile uint8_t seven_seg_cc = 0;

/* Seven segment display segment values for 0 to 9 */
uint8_t seven_seg_data[10] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};

int interrupt() {
    /* Make all bits of port A and the least significant
    ** bit of port C be output bits.
    */
    /* YOUR CODE HERE */
    DDRA = 0xFF;
    DDRC = 0x01;

    /* Set up timer/counter 1 so that we get an
    ** interrupt 100 times per second, i.e. every
    ** 10 milliseconds.
    */
    OCR1A = 10000 - 1;  // We want to count up to 10000 (10ms at 1MHz), but the count starts at 0,
                        // so we set OCR1A to 9999
    // TCCR1A = 1 << WGM12;  // CTC mode - count up to OCR1A, then reset to 0 and trigger interrupt
    TCCR1A = 0;  // CTC mode - count up to OCR1A, then reset to 0 and trigger interrupt
    TCCR1B = 1 << CS11 | 1 << WGM12;  // clk/8 prescaler - with 8MHz clock, this gives us a 1MHz
                                      // timer clock, so we need OCR1A to be 10000 for a 10ms period

    /* Enable interrupt on timer on output compare match
     */
    TIMSK1 = 1 << OCIE1A;

    /* Ensure interrupt flag is cleared */
    TIFR1 = 1 << OCF1A;

    /* Turn on global interrupts */
    sei();

    /* Implement controller here - RESET functionality is provided */
    /* See button macro definitions above */
    while (1) {
        if (RESET) {
            /* Reset button has been pushed, wait for it to be released */
            while (RESET) {
                /* Do nothing - start/stop button will be ignored */
            }
            /* RESET button has been released - now reset the stopwatch */
            stopwatch_timing = 0;
            digits_displayed = 0;
            count = 0;
        }
        /* Deal with start/stop button */
        /* YOUR CODE HERE */

        if (STARTSTOP) {
            while (STARTSTOP) {
            }
            stopwatch_timing = !stopwatch_timing;
            digits_displayed = 1;
        }
    }
}

/* This interrupt handler will get called every 10ms.
** We do two things - update out stopwatch count, and
** output to the other seven segment display digit.
** We display seconds on the left digit;
** tenths of seconds on the right digit.
*/
ISR(TIMER1_COMPA_vect) {
    /* If the stopwatch is running then increment time.
    ** If we've reached 1000, then wrap this around to 0.
    */
    if (stopwatch_timing) {
        count++;
        if (count == 1000) {
            count = 0;
        }
    }

    /* Change which digit will be displayed. If last time was
    ** left, now display right. If last time was right, now
    ** display left.
    */
    seven_seg_cc = !seven_seg_cc;

    if (digits_displayed) {
        /* Display a digit */
        if (seven_seg_cc == 0) {
            /* Display rightmost digit - tenths of seconds */
            PORTA = seven_seg_data[(count / 10) % 10];
        } else {
            /* Display leftmost digit - seconds + decimal point */
            PORTA = seven_seg_data[count / 100] | (1 << 7);
        }
        /* Output the digit selection (CC) bit */
        PORTC = seven_seg_cc;
    } else {
        /* No digits displayed -  display is blank */
        PORTA = 0x00;
    }
}

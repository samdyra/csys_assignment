/*
 * morse.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Bradley Stone, Ryan Wang
 * Modified by Dwiputra Sam, 49804980
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>

#include "display.h"
#include "encoding.h"
#include "input.h"
#include "io_leds.h"
#include "ledmatrix.h"
#include "serialio.h"
#include "seven_segment.h"
#include "terminalio.h"

// MODELS

/* TIMER */
// shared counter, accessible by using extern in other files
volatile uint32_t shared_counter_0 = 0;

// INPUT STATES
// mark is a dash or dot, used for determining end of letter
static uint8_t has_mark_in_current_char = 0;
// count how many consecutive submits since last mark
static uint8_t consecutive_submits = 0;

/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void main_loop(void);
void handle_dot(void);
void handle_dash(void);
void handle_submit(void);
void initialize_timer_0(void);
void handle_char_from_serial(char c);

// initialize timer and button inputs
void initialize_timer_0(void) {
    TIMSK0 = (1 << OCIE0A);              // enable compare match interrupt
    TCCR0A = (1 << WGM01);               // ctc mode
    TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler 64
    OCR0A = 249;                         // 250 ticks × 8us = 2ms
}

// run everything
int main(void) {
    initialise_hardware();
    start_splash_screen();
    start_morse();
}

void initialise_hardware(void) {
    spi_setup_master(128);  // init LED matrix
    // Setup serial port for 19200 baud communication
    init_serial_stdio(19200);
    input_init();
    initialize_uq_io_board_led();
    seven_segment_init();
    initialize_timer_0();
    sei();  // enable global interrupts
}

void start_splash_screen(void) {
    // draw sigil on LED matrix
    start_splash_display();
    move_terminal_cursor(10, 6);
    printf("CSSE%d AVR Project", 7201);
    move_terminal_cursor(10, 8);
    printf("\"Morse Code Emulator\"");
    move_terminal_cursor(10, 10);
    printf("%d, Semester %s", 2026, "One");
    move_terminal_cursor(10, 12);
    printf("By %s (%ld)", "Dwiputra Sam", 49804980);

    while (!(PINB & 0x07)) {
        ;  // do nothing til button press
    }
    ledmatrix_clear();
}

void start_morse(void) {
    clear_terminal();
    main_loop();
}

// RUNTIMES
// main runtime
void main_loop(void) {
    while (1) {
        // handle any button press/hold
        listen_button_input();

        // handle any serial input
        listen_serial_input();

        // animate uq io board leds
        render_uq_io_board_led();

        // animate led matrix
        render_led_matrix();
    }
}

// 2ms counter runtime
ISR(TIMER0_COMPA_vect) {
    shared_counter_0 += 2;
    render_seven_segment();
}

/* EVENT DISPATCHERS */

void handle_dot(void) {
    handle_dot_input_in_uq_io_led(has_mark_in_current_char);
    handle_dot_input_led_matrix();
    handle_dot_input_in_seven_segment();

    has_mark_in_current_char = 1;
    consecutive_submits = 0;
}

void handle_dash(void) {
    handle_dash_input_in_uq_io_led(has_mark_in_current_char);
    handle_dash_input_led_matrix();
    handle_dash_input_in_seven_segment();

    has_mark_in_current_char = 1;
    consecutive_submits = 0;
}

void handle_submit(void) {
    // early return for 3rd+ submit
    if (consecutive_submits > 1) return;

    handle_submit_input_in_uq_io_led(consecutive_submits);
    handle_submit_input_led_matrix();
    handle_submit_input_in_seven_segment();

    consecutive_submits++;
    has_mark_in_current_char = 0;
}

void handle_char_from_serial(char character) {
    // convert lowercase to uppercase
    if (character >= 'a' && character <= 'z') {
        character = character - 'a' + 'A';
    }

    uint8_t encoding = char_to_morse(character);
    if (encoding == 0) return;  // invalid character do nothing

    // suppress a serial space if we're already at the space-displayed state
    if (character == ' ' && consecutive_submits >= 2) return;

    // echo to terminal
    printf("%c", character);

    // dispatch to interfaces
    handle_serial_char_in_uq_io_led(encoding);
    handle_serial_char_led_matrix(character);
    handle_serial_char_in_seven_segment();

    has_mark_in_current_char = 0;  // reset shared state, any mark (if any) is abandoned

    if (character == ' ') {
        consecutive_submits = 2;  // serial space = "the space happened" already
    } else {
        consecutive_submits = 1;  // serial char also acts as a "submit" one more press = space
    }
}

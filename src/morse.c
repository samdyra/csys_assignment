/*
 * morse.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Bradley Stone, Ryan Wang
 * Modified by Dwiputra Sam, 49804980
 */

/* Definitions */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>

/* Internal Library Includes */
#include "display.h"
#include "encoding.h"
#include "ledmatrix.h"
#include "serialio.h"
#include "terminalio.h"

// MODELS

/* UQ IO BOARD LED DATA MODELS */
// persistence layer that holds led state
static uint8_t led_pattern = 0x00;
// mark is a dash or dot, used for determining end of letter
static uint8_t has_mark_in_current_char = 0;
// count how many consecutive
static uint8_t consecutive_submits = 0;

/* LED MATRIX DATA MODELS */
// store an in progress char creation (0 for dot, 1 for dash) (see morse_to_char)
static uint8_t current_char_encoding = 0b1;
// store the latest created char after user submit
static char latest_generated_char = 0;

/* Internal Function Declarations */
void initialise_hardware(void);
void start_morse(void);
void start_splash_screen(void);
void handle_button_input(void);
void handle_inputs(void);
void handle_dot(void);
void handle_dash(void);
void handle_submit(void);
void update_io_leds(void);
void update_led_matrix(void);

int main(void) {
    initialise_hardware();
    start_splash_screen();
    start_morse();
}

void initialise_hardware(void) {
    spi_setup_master(128);  // init LED matrix
    // Setup serial port for 19200 baud communication
    init_serial_stdio(19200);
    // setup buttons for io board led
    DDRB &= ~((1 << DDRB0) | (1 << DDRB1) | (1 << DDRB2));  // clear b0, b1, b2 to be inputs
    DDRA |= (1 << DDRA0) | (1 << DDRA1) | (1 << DDRA2) |
            (1 << DDRA3);  // set a0, a1, a2, a3 to be outputs
    DDRD |= (1 << DDRD5) | (1 << DDRD4) | (1 << DDRD3) |
            (1 << DDRD2);  // set d3, d4, d5, d2 to be outputs
    sei();                 // enable global interrupts
}

void start_splash_screen(void) {
    // draw sigil on LED matrix
    start_splash_display();
    move_terminal_cursor(10, 6);
    printf("CSSE%d AVR Project", 7210);  // change if masters student
    move_terminal_cursor(10, 8);
    printf("\"Morse Code Emulator\"");
    move_terminal_cursor(10, 10);
    printf("%d, Semester %s", 2026, "One");
    move_terminal_cursor(10, 12);
    // "%ld" is "long decimal", since a student number is bigger than 2**16
    printf("By %s (%ld)", "Dwiputra Sam", 49804980);

    // Wait until a button is pressed
    while (!(PINB & 0x07)) {
        ;  // do nothing til button press
    }
    ledmatrix_clear();
}

void start_morse(void) {
    // Clear the serial terminal
    clear_terminal();

    while (1) {
        // Handle any button or key inputs
        handle_inputs();
    }
    // should never reach
}

void handle_inputs(void) {
    /* ******** START HERE ********

    Read the button. Enter a mark if there is a rising edge on b0.
    A way to do this is to check if the previous b0 state is 0,
    and the current b0 state is a 1.
        (You will need to implement a method of tracking the previous b0 state.)
        Ensure that when you press a button to exit the splash screen,
        that this button press doesn't immediately trigger an input here.

    --. --- --- -.. / .-.. ..- -.-. -.-
    */

    handle_button_input();
}

/* MODEL EVENT LISTENERS */
// IO Button Event listeners
void handle_button_input(void) {
    static uint8_t prev = 0b000;
    uint8_t curr = PINB & ((1 << PINB2) | (1 << PINB1) | (1 << PINB0));
    uint8_t edges = curr & ~prev;
    prev = curr;

    if (edges & (1 << 0)) handle_dot();
    if (edges & (1 << 1)) handle_dash();
    if (edges & (1 << 2)) handle_submit();
}

// B0
void handle_dot(void) {
    // uq io board led handling
    if (has_mark_in_current_char) {
        led_pattern = (led_pattern << 2) | 0b01;
    } else {
        led_pattern = (led_pattern << 1) | 0b01;
    }
    has_mark_in_current_char = 1;
    consecutive_submits = 0;

    update_io_leds();

    // led matrix handling
    current_char_encoding = current_char_encoding << 1 | 0b0;
}

// B1
void handle_dash(void) {
    // uq io board led handling
    if (has_mark_in_current_char) {
        led_pattern = (led_pattern << 4) | 0b111;
    } else {
        led_pattern = (led_pattern << 3) | 0b111;
    }
    has_mark_in_current_char = 1;
    consecutive_submits = 0;

    update_io_leds();

    // led matrix handling
    current_char_encoding = current_char_encoding << 1 | 0b1;
}

// B2
void handle_submit(void) {
    // UQ IO LED Board handler
    if (consecutive_submits > 1) {
        // nothng
        return;
    } else if (consecutive_submits == 1) {
        led_pattern = led_pattern << 2;
        consecutive_submits++;
        has_mark_in_current_char = 0;
    } else if (consecutive_submits == 0) {
        led_pattern = led_pattern << 3;
        consecutive_submits++;
        has_mark_in_current_char = 0;
    }

    update_io_leds();

    // LED Matrix handler
    latest_generated_char = morse_to_char(current_char_encoding);
    current_char_encoding = 0b1;  // reset for next char
    update_led_matrix();
}

/* UI INTERFACE VIEWS */
void update_io_leds(void) {
    // port A = lower half of led
    PORTA = (PORTA & 0xF0) | (led_pattern & 0x0F);
    // port B = uppper half of led
    PORTD = (PORTD & 0b11000011) | ((led_pattern & 0xF0) >> 2);
}

void update_led_matrix(void) {
    if (latest_generated_char != 0) {  // skip if nothing submitted yet
        // shift left 4 bits
        ledmatrix_shift_display(SHIFT_LEFT);
        ledmatrix_shift_display(SHIFT_LEFT);
        ledmatrix_shift_display(SHIFT_LEFT);
        ledmatrix_shift_display(SHIFT_LEFT);

        uint8_t GLYPH_WIDTH = 3;

        draw_small_char(latest_generated_char, MATRIX_NUM_COLUMNS - GLYPH_WIDTH, COLOUR_GREEN);
    }
}
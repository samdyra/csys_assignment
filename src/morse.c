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
#include "seven_segment.h"
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
// store all pending led matrix left shifts
static uint8_t pending_matrix_shifts = 0;
// the time of last time the matrix shift
static uint32_t last_time_matrix_shift = 0;

/* TIMER */
static volatile uint32_t shared_counter_0 = 0;

/* SEVEN SEGMENT DISPLAY */
// marks in in progress char
static volatile uint8_t seven_seg_mark_count = 0;
// total chars submitted
static volatile uint8_t seven_seg_submitted_count = 0;

/* IO LED ANIMATION QUEUE */
#define TICK_QUEUE_SIZE 32
static uint8_t tick_queue[TICK_QUEUE_SIZE];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;
static uint8_t queue_count = 0;
static uint32_t last_time_io_led_tick = 0;

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
void update_incomplete_char(void);
void enqueue_tick(uint8_t tick);
uint8_t dequeue_tick(void);
void snap_matrix_animation(void);

int main(void) {
    initialise_hardware();
    start_splash_screen();
    start_morse();
}

void initialize_timer_0(void) {
    TIMSK0 = (1 << OCIE0A);              // enable compare match interrupt
    TCCR0A = (1 << WGM01);               // ctc mode
    TCCR0B = (1 << CS01) | (1 << CS00);  // prescaler 64
    OCR0A = 249;                         // 250 ticks × 8us = 2ms
}

void initialize_button_inputs(void) {
    DDRB &= ~((1 << DDRB0) | (1 << DDRB1) | (1 << DDRB2));  // clear b0, b1, b2 to be inputs
    DDRA |= (1 << DDRA0) | (1 << DDRA1) | (1 << DDRA2) |
            (1 << DDRA3);  // set a0, a1, a2, a3 to be outputs
    DDRD |= (1 << DDRD5) | (1 << DDRD4) | (1 << DDRD3) |
            (1 << DDRD2);  // set d3, d4, d5, d2 to be outputs
}

void initialise_hardware(void) {
    spi_setup_master(128);  // init LED matrix
    // Setup serial port for 19200 baud communication
    init_serial_stdio(19200);
    seven_segment_init();
    initialize_button_inputs();
    initialize_timer_0();
    sei();  // enable global interrupts
}

void start_splash_screen(void) {
    // draw sigil on LED matrix
    start_splash_display();
    move_terminal_cursor(10, 6);
    printf("CSSE%d AVR Project", 7201);  // change if masters student
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

        // animate uq io board leds
        if (shared_counter_0 - last_time_io_led_tick >= 100) {
            update_io_leds();
        }

        // animate led matrix
        if (shared_counter_0 - last_time_matrix_shift >= 100) {
            if (pending_matrix_shifts > 0) {
                ledmatrix_shift_display(SHIFT_LEFT);
                pending_matrix_shifts--;
                last_time_matrix_shift = shared_counter_0;
            }
        }
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
    // dot input in UQ IO LED
    if (has_mark_in_current_char) {
        enqueue_tick(0);
        enqueue_tick(1);
    } else {
        enqueue_tick(1);

        has_mark_in_current_char = 1;
    }

    consecutive_submits = 0;

    update_io_leds();

    // led matrix handling on click dot
    current_char_encoding = (current_char_encoding << 1) | 0b0;
    update_incomplete_char();

    // seven segment display handling
    seven_seg_mark_count++;
}

// B1
void handle_dash(void) {
    // dash input in UQ IO LED
    if (has_mark_in_current_char) {
        enqueue_tick(0);
        enqueue_tick(1);
        enqueue_tick(1);
        enqueue_tick(1);
    } else {
        enqueue_tick(1);
        enqueue_tick(1);
        enqueue_tick(1);

        has_mark_in_current_char = 1;
    }

    consecutive_submits = 0;

    update_io_leds();

    // led matrix handling on click dash
    current_char_encoding = (current_char_encoding << 1) | 0b1;
    update_incomplete_char();

    // seven segment display handling
    seven_seg_mark_count++;
}

// B2
void handle_submit(void) {
    // submit input in UQ IO LED
    if (consecutive_submits > 1) return;  // if its 2, do nothng

    if (consecutive_submits == 0) {
        enqueue_tick(0);
        enqueue_tick(0);
        enqueue_tick(0);
    } else if (consecutive_submits == 1) {
        enqueue_tick(0);
        enqueue_tick(0);
    }

    consecutive_submits++;
    has_mark_in_current_char = 0;

    update_io_leds();

    // led matrix handler on submit
    latest_generated_char = morse_to_char(current_char_encoding);
    current_char_encoding = 0b1;
    update_led_matrix();

    // seven segment display handling
    seven_seg_submitted_count++;
    seven_seg_mark_count = 0;
}

// on compare match timer 0 per 2ms
ISR(TIMER0_COMPA_vect) {
    shared_counter_0 += 2;
    seven_segment_step(seven_seg_mark_count, seven_seg_submitted_count);
}

/* UI INTERFACE VIEWS */
void update_io_leds(void) {
    if (queue_count == 0) return;  // nothing to animate

    uint8_t tick = dequeue_tick();
    led_pattern = (led_pattern << 1) | tick;
    // port A = lower half of led
    PORTA = (PORTA & 0xF0) | (led_pattern & 0x0F);
    // port B = uppper half of led
    PORTD = (PORTD & 0b11000011) | ((led_pattern & 0xF0) >> 2);

    last_time_io_led_tick = shared_counter_0;
}

void update_led_matrix(void) {
    snap_matrix_animation();           // finish previous animation if any
    if (latest_generated_char != 0) {  // skip if nothing submitted yet
        draw_small_char(latest_generated_char, MATRIX_NUM_COLUMNS - GLYPH_WIDTH, COLOUR_GREEN);
        pending_matrix_shifts = 4;
        last_time_matrix_shift = shared_counter_0;
    }
}

void update_incomplete_char(void) {
    snap_matrix_animation();  // finish previous animation if any
    char incomplete_char = morse_to_char(current_char_encoding);
    draw_small_char(incomplete_char, MATRIX_NUM_COLUMNS - GLYPH_WIDTH, COLOUR_RED);
}

/* HELPER FUNCTIONS */
void enqueue_tick(uint8_t tick) {
    if (queue_count < TICK_QUEUE_SIZE) {
        // write the new tick at the tail position.
        // tick & 1 ensures only store 1 bit 0 or 1, never accidentally larger
        tick_queue[queue_tail] = tick & 1;

        // make the array circular
        // Advance tail by 1, wrapping back to 0 if it would go past the end.
        //  ex: queue_tail = 30 → (30+1) % 32 = 31    (normal step)
        // exL  queue_tail = 31 → (31+1) % 32 = 0     (wraps to start)
        queue_tail = (queue_tail + 1) % TICK_QUEUE_SIZE;

        queue_count++;
    }
}

uint8_t dequeue_tick(void) {
    // Read the oldest tick (head).
    uint8_t tick = tick_queue[queue_head];

    // Advance head by 1, wrapping back to 0 the same way as tail.
    // head will chase tail, the range of head and tail = queue count
    queue_head = (queue_head + 1) % TICK_QUEUE_SIZE;

    queue_count--;
    return tick;
}

// finish in progress led matrix animation to its final state.
// used before drawing a new character to keep the right edge clean.
void snap_matrix_animation(void) {
    while (pending_matrix_shifts > 0) {
        ledmatrix_shift_display(SHIFT_LEFT);
        pending_matrix_shifts--;
    }
}
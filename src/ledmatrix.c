#include "ledmatrix.h"

#include <avr/io.h>
#include <stdint.h>

#include "display.h"
#include "encoding.h"
#include "spi.h"

#define CMD_UPDATE_COL 0x03
#define CMD_CLEAR_SCREEN 0x0F
#define CMD_SHIFT_DISPLAY 0x04

/* Internal Function Declarations */
static void update_led_matrix(void);
static void update_incomplete_char(void);
static void snap_matrix_animation(void);

// MODELS
// access morse.c's timer counter
extern volatile uint32_t shared_counter_0;
// store an in progress char creation (0 for dot, 1 for dash) (see morse_to_char)
static uint8_t current_char_encoding = 0b1;
// store the latest created char after user submit
static char latest_generated_char = 0;
// store all pending led matrix left shifts
static uint8_t pending_matrix_shifts = 0;
// the time of last time the matrix shift
static uint32_t last_time_matrix_shift = 0;

/* UI INTERFACE VIEWS */
void render_led_matrix(void) {
    if (shared_counter_0 - last_time_matrix_shift >= 100) {
        if (pending_matrix_shifts > 0) {
            ledmatrix_shift_display(SHIFT_LEFT);
            pending_matrix_shifts--;
            last_time_matrix_shift = shared_counter_0;
        }
    }
}

// CONTROLLERS
static void update_led_matrix(void) {
    snap_matrix_animation();
    if (latest_generated_char != 0) {
        draw_small_char(latest_generated_char, MATRIX_NUM_COLUMNS - GLYPH_WIDTH, COLOUR_GREEN);
        pending_matrix_shifts = 4;
        last_time_matrix_shift = shared_counter_0;
    }
}

static void update_incomplete_char(void) {
    snap_matrix_animation();
    char incomplete_char = morse_to_char(current_char_encoding);
    draw_small_char(incomplete_char, MATRIX_NUM_COLUMNS - GLYPH_WIDTH, COLOUR_RED);
}

// finish in progress led matrix animation to its final state.
// used before drawing a new character to keep the right edge clean.
static void snap_matrix_animation(void) {
    while (pending_matrix_shifts > 0) {
        ledmatrix_shift_display(SHIFT_LEFT);
        pending_matrix_shifts--;
    }
}

void handle_dot_input_led_matrix(void) {
    // led matrix handling on click dot
    current_char_encoding = (current_char_encoding << 1) | 0b0;
    update_incomplete_char();
}

void handle_dash_input_led_matrix(void) {
    // led matrix handling on click dash
    current_char_encoding = (current_char_encoding << 1) | 0b1;
    update_incomplete_char();
}

void handle_submit_input_led_matrix(void) {
    // led matrix handler on submit
    latest_generated_char = morse_to_char(current_char_encoding);
    current_char_encoding = 0b1;
    update_led_matrix();
}

// UTILS

void ledmatrix_update_column(uint8_t x, uint8_t pixels[MATRIX_NUM_ROWS]) {
    if (x >= MATRIX_NUM_COLUMNS) {
        return;
    }
    (void)spi_send_byte(CMD_UPDATE_COL);
    (void)spi_send_byte(x & 0x0F);
    for (uint8_t y = 0; y < MATRIX_NUM_ROWS; y++) {
        (void)spi_send_byte(pixels[y]);
    }
}

void ledmatrix_clear(void) { (void)spi_send_byte(CMD_CLEAR_SCREEN); }

void ledmatrix_shift_display(uint8_t direction) {
    spi_send_byte(CMD_SHIFT_DISPLAY);
    spi_send_byte(direction);
}
#include "ledmatrix.h"

#include <avr/io.h>
#include <stdint.h>

#include "display.h"
#include "encoding.h"
#include "spi.h"

#define CMD_UPDATE_COL 0x03
#define CMD_CLEAR_SCREEN 0x0F
#define CMD_SHIFT_DISPLAY 0x04

#define HISTORY_SIZE 50

/* Internal Function Declarations */
static void update_led_matrix(void);
static void update_incomplete_char(void);
static void snap_matrix_animation(void);
static void draw_char_in_current_font(char c, uint8_t x, uint8_t colour);
static void push_to_history(char c, uint8_t colour);

// MODELS
// access morse.c's timer counter
extern volatile uint32_t shared_counter_0;
// store an in progress char creation (0 for dot, 1 for dash) (see morse_to_char)
static uint8_t current_char_encoding = 0b1;  // all char encoding start with 1
// store the latest created char after user submit
static char latest_generated_char = 0;
// store all pending led matrix left shifts
static uint8_t pending_matrix_shifts = 0;
// the time of last time the matrix shift
static uint32_t last_time_matrix_shift = 0;

// store all char history in a buffer
static char char_history[HISTORY_SIZE];
// store all char color history in a buffer
static uint8_t color_history[HISTORY_SIZE];
static uint32_t total_num_char_submitted = 0;

// 0 = small (3-col glyph + 1 gap = 4 col stride)
// 1 = large (5-col glyph + 1 gap = 6 col stride)
static uint8_t current_font = 0;
static uint8_t glyph_width = GLYPH_WIDTH_SMALL;  // small by default
static uint8_t glyph_width_and_space = 4;        // width + 1 (space)
static uint8_t max_chars_visible = 3;            // small font = 3, big font 1

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

// re-render the LED matrix using the chars currently in the buffer.
// hit on switching or scrolling
void redraw_matrix_from_buffer(void) {
    // wipe the matrix and cancel any in-flight shift animation
    ledmatrix_clear();
    pending_matrix_shifts = 0;

    // total x minus a char and a char + space (glyph_width is for the char itself and
    // glyph_width_and_space for new chars)
    uint8_t rightmost_char_x = MATRIX_NUM_COLUMNS - glyph_width - glyph_width_and_space;

    // figure out how many chars to draw
    uint8_t num_of_chars_to_draw =
        total_num_char_submitted < max_chars_visible ? total_num_char_submitted : max_chars_visible;
    // figure out leftest x to start
    // -1 because its the total num of char between l r
    uint8_t leftmost_char_x = rightmost_char_x - (num_of_chars_to_draw - 1) * glyph_width_and_space;
    // oldest in buf = total num - num of chars should be rendered
    uint32_t oldest_visible_index_buf = total_num_char_submitted - num_of_chars_to_draw;

    // iterate left-to-right: oldest visible char first, ending at the newest
    for (uint8_t i = 0; i < num_of_chars_to_draw; i++) {
        uint32_t char_index_in_buf = oldest_visible_index_buf + i;  // start from 0 first char
        uint8_t buf_idx = char_index_in_buf % HISTORY_SIZE;

        uint8_t x = leftmost_char_x + i * glyph_width_and_space;  // x coord in led
        draw_char_in_current_font(char_history[buf_idx], x, color_history[buf_idx]);
    }

    // user is typing
    // overlay the incomplete-char preview at the right edge in red
    if (current_char_encoding != 0b1) {
        char incomplete = morse_to_char(current_char_encoding);
        draw_char_in_current_font(incomplete, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_RED);
    }
}

// CONTROLLERS
static void update_led_matrix(void) {
    snap_matrix_animation();
    if (latest_generated_char != 0) {
        draw_char_in_current_font(latest_generated_char, MATRIX_NUM_COLUMNS - glyph_width,
                                  COLOUR_GREEN);
        pending_matrix_shifts = glyph_width_and_space;
        last_time_matrix_shift = shared_counter_0;
    }
}

static void update_incomplete_char(void) {
    snap_matrix_animation();
    char incomplete_char = morse_to_char(current_char_encoding);
    draw_char_in_current_font(incomplete_char, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_RED);
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
    push_to_history(latest_generated_char, COLOUR_GREEN);
}

void handle_serial_char_led_matrix(char c) {
    snap_matrix_animation();      // finish any pending animation
    current_char_encoding = 0b1;  // reset in-progress encoding (abandon button input)
    draw_char_in_current_font(c, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_YELLOW);
    pending_matrix_shifts = glyph_width_and_space;
    last_time_matrix_shift = shared_counter_0;
    push_to_history(c, COLOUR_YELLOW);
}

// UTILS

void set_font(uint8_t font) {
    current_font = font;
    if (font == 0) {
        glyph_width = GLYPH_WIDTH_SMALL;
        glyph_width_and_space = GLYPH_WIDTH_SMALL + 1;
        max_chars_visible = 3;
    } else {
        glyph_width = GLYPH_WIDTH_LARGE;
        glyph_width_and_space = GLYPH_WIDTH_LARGE + 1;
        max_chars_visible = 1;  // only 1 large char fits given current layout
    }
}

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

static void draw_char_in_current_font(char character, uint8_t x, uint8_t colour) {
    if (current_font == 0) {
        draw_small_char(character, x, colour);
    } else {
        draw_large_char(character, x, colour);
    }
}

static void push_to_history(char c, uint8_t colour) {
    uint8_t idx = total_num_char_submitted % HISTORY_SIZE;
    char_history[idx] = c;
    color_history[idx] = colour;
    total_num_char_submitted++;
}

void ledmatrix_clear(void) { (void)spi_send_byte(CMD_CLEAR_SCREEN); }

void ledmatrix_shift_display(uint8_t direction) {
    spi_send_byte(CMD_SHIFT_DISPLAY);
    spi_send_byte(direction);
}
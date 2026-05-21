#include "morse_led_display.h"

#include <avr/io.h>
#include <stdint.h>

#include "display.h"
#include "eeprom.h"
#include "encoding.h"
#include "ledmatrix.h"
#include "serial_output.h"

#define HISTORY_SIZE 50

/* Internal Function Declarations */
static void draw_glyph_column(char c, uint8_t col_in_glyph, uint8_t colour, uint8_t matrix_col);
static void draw_char_in_current_font(char c, uint8_t x, uint8_t colour);
static void draw_matrix_col_from_buffer(uint8_t matrix_col);
static void update_incomplete_char(void);
static void finish_in_progress_animation(void);
static void push_to_history_buf(char c, uint8_t colour);
static uint16_t max_scroll_offset(void);

// MODEL
// access morse.c's timer counter
extern volatile uint32_t shared_counter_0;

// in-progress char encoding (0 for dot, 1 for dash) (see morse_to_char)
static uint8_t current_char_encoding = 0b1;  // all char encoding start with 1
// the latest char created after user submit
static char latest_generated_char = 0;
// pending left shifts queued by the slide-in animation
static uint8_t pending_matrix_shifts = 0;
// the last time the matrix shifted (for 100ms animation cadence)
static uint32_t last_time_matrix_shift = 0;

// char history (last HISTORY_SIZE chars submitted)
static char char_buffer_history[HISTORY_SIZE];
static uint8_t color_buffer_history[HISTORY_SIZE];
static uint32_t total_num_char_submitted = 0;

// 0 = small 3 col glyph + 1 gap
// 1 = large 5-col glyph + 1 gap
static uint8_t current_font = 0;
static uint8_t glyph_width = GLYPH_WIDTH_SMALL;
static uint8_t glyph_width_and_space = 4;

// scroll offset in columns. 0 = present view (newest at rightmost position).
// positive values shift the view into the past.
static uint16_t scroll_offset_cols = 0;

// VIEWSS

// animation tick. slides chars left one col every 100ms until all
// pending shifts are completed.
void left_shift_led_matrix(void) {
    if (shared_counter_0 - last_time_matrix_shift >= 100) {
        if (pending_matrix_shifts > 0) {
            ledmatrix_shift_display(SHIFT_LEFT);
            pending_matrix_shifts--;
            last_time_matrix_shift = shared_counter_0;
        }
    }
}

// instantly finish any in-progress slide animation to its final state.
// used before drawing a new character to keep the right edge clean.
static void finish_in_progress_animation(void) {
    while (pending_matrix_shifts > 0) {
        ledmatrix_shift_display(SHIFT_LEFT);
        pending_matrix_shifts--;
    }
}

// dispatch to small or large glyph column draw based on current font.
static void draw_glyph_column(char c, uint8_t col_in_glyph, uint8_t colour, uint8_t matrix_col) {
    if (current_font == 0) {
        draw_small_glyph_column(c, col_in_glyph, colour, matrix_col);
    } else {
        draw_large_glyph_column(c, col_in_glyph, colour, matrix_col);
    }
}

// dispatch to small or large draw_char based on current font.
static void draw_char_in_current_font(char character, uint8_t x, uint8_t colour) {
    if (current_font == 0) {
        draw_small_char(character, x, colour);
    } else {
        draw_large_char(character, x, colour);
    }
}

// CONTROLLERS

// draw the given col in led matrix (0-15) based on scroll_offset_cols and char buffer history
static void draw_matrix_col_from_buffer(uint8_t matrix_col) {
    if (total_num_char_submitted == 0) return;

    // rightmost matrix col of the newest char
    int32_t rightmost_x_no_scroll = MATRIX_NUM_COLUMNS - glyph_width - glyph_width_and_space;

    // timeline position of the newest char (no progress char)
    int32_t rightmost_x_no_scroll_timeline =
        (int32_t)(total_num_char_submitted - 1) * glyph_width_and_space;

    // matrix leftmost col position in timeline
    int32_t matrix_left_edge_col_on_timeline =
        rightmost_x_no_scroll_timeline - rightmost_x_no_scroll;

    // scroll_offset shifts this further into the past.
    int32_t left_edge_offset_adjusted =
        matrix_left_edge_col_on_timeline - (int32_t)scroll_offset_cols;

    // timeline position of this specific matrix col
    int32_t timeline_col = (int32_t)matrix_col + left_edge_offset_adjusted;

    if (timeline_col < 0) return;  // before any char existed

    // char index in buf (rounded down)
    uint32_t char_index_buf = (uint32_t)timeline_col / glyph_width_and_space;
    uint8_t col_in_led = (uint8_t)((uint32_t)timeline_col % glyph_width_and_space);

    // checks
    if (char_index_buf >= total_num_char_submitted) return;  // past the newest
    if (col_in_led >= glyph_width) return;                   // space between chars

    // circ buffer wrap
    uint8_t index_in_buf = char_index_buf % HISTORY_SIZE;

    char char_from_buf = char_buffer_history[index_in_buf];
    uint8_t color_from_buf = color_buffer_history[index_in_buf];

    draw_glyph_column(char_from_buf, col_in_led, color_from_buf, matrix_col);
}

// draw the incomplete-char preview at the right edge in red.
static void update_incomplete_char(void) {
    finish_in_progress_animation();
    char incomplete_char = morse_to_char(current_char_encoding);
    draw_char_in_current_font(incomplete_char, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_RED);
}

// full re-render of the matrix from the buffer and current scroll offset.
// called when something changes what's on-screen without animation —
// font switch or scroll snap.
void redraw_matrix_from_buffer(void) {
    ledmatrix_clear();
    // cancel any in progress slide animation so it doesn't shift our fresh render
    pending_matrix_shifts = 0;

    // column-by-column rendering — naturally handles partial chars
    // at the edges when scrolled mid-character.
    for (uint8_t matrix_col = 0; matrix_col < MATRIX_NUM_COLUMNS; matrix_col++) {
        draw_matrix_col_from_buffer(matrix_col);
    }

    // incomplete-char preview only at present view (when scrolled back,
    // the right edge belongs to old content, not the in-progress char)
    if (scroll_offset_cols == 0 && current_char_encoding != 0b1) {
        char incomplete_char = morse_to_char(current_char_encoding);
        draw_char_in_current_font(incomplete_char, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_RED);
    }
}

// if currently viewing scrollback, return to the present view and redraw.
void snap_to_present(void) {
    if (scroll_offset_cols == 0) return;  // so not rerender everything everytime

    scroll_offset_cols = 0;
    redraw_matrix_from_buffer();
}

// EVENT HANDLER

void handle_dot_input_led_matrix(void) {
    // led matrix handling on click dot
    snap_to_present();
    current_char_encoding = (current_char_encoding << 1) | 0b0;
    update_incomplete_char();
}

void handle_dash_input_led_matrix(void) {
    // led matrix handling on click dash
    snap_to_present();
    current_char_encoding = (current_char_encoding << 1) | 0b1;
    update_incomplete_char();
}

void handle_submit_input_led_matrix(void) {
    // led matrix handler on submit
    snap_to_present();
    latest_generated_char = morse_to_char(current_char_encoding);
    current_char_encoding = 0b1;
    push_to_history_buf(latest_generated_char, COLOUR_GREEN);

    finish_in_progress_animation();
    // draw the latest submitted char at the right edge in green,
    // then queue the slide-left animation.
    if (latest_generated_char != 0) {
        draw_char_in_current_font(latest_generated_char, MATRIX_NUM_COLUMNS - glyph_width,
                                  COLOUR_GREEN);
        pending_matrix_shifts = glyph_width_and_space;
        last_time_matrix_shift = shared_counter_0;
    }
}

// handle serial terminal input to led matrix
void handle_serial_char_led_matrix(char c) {
    snap_to_present();
    finish_in_progress_animation();  // finish any pending animation
    current_char_encoding = 0b1;     // reset in-progress encoding (abandon button input)
    draw_char_in_current_font(c, MATRIX_NUM_COLUMNS - glyph_width, COLOUR_YELLOW);
    pending_matrix_shifts = glyph_width_and_space;
    last_time_matrix_shift = shared_counter_0;
    push_to_history_buf(c, COLOUR_YELLOW);
}

void scroll_one_col_into_past(void) {
    if (scroll_offset_cols >= max_scroll_offset()) return;  // at oldest-visible boundary
    scroll_offset_cols++;
    // shift display right (content moves right, col 0 clears) — older content slides in from left
    ledmatrix_shift_display(SHIFT_RIGHT);
    draw_matrix_col_from_buffer(0);  // render the leftmost
}

void scroll_one_col_toward_present(void) {
    if (scroll_offset_cols == 0) return;  // already at present
    scroll_offset_cols--;
    // shift display left (content moves left, col 15 clears) — newer content slides in from right
    ledmatrix_shift_display(SHIFT_LEFT);
    draw_matrix_col_from_buffer(MATRIX_NUM_COLUMNS - 1);  // render the rightmost

    // show the inprogress char col
    if (current_char_encoding != 0b1 && scroll_offset_cols < glyph_width) {
        char incomplete_char = morse_to_char(current_char_encoding);
        uint8_t preview_col_index = glyph_width - 1 - scroll_offset_cols;
        draw_glyph_column(incomplete_char, preview_col_index, COLOUR_RED, MATRIX_NUM_COLUMNS - 1);
    }
}

// UTILS

// event handler called in input.c
void set_font(uint8_t font) {
    current_font = font;
    if (font == 0) {
        // small font: 3 col glyph + 1 gap
        glyph_width = GLYPH_WIDTH_SMALL;
        glyph_width_and_space = GLYPH_WIDTH_SMALL + 1;
    } else {
        // large font: 5-col glyph + 1 gap
        glyph_width = GLYPH_WIDTH_LARGE;
        glyph_width_and_space = GLYPH_WIDTH_LARGE + 1;
    }
    scroll_offset_cols = 0;  // font change resets scroll
}

static void push_to_history_buf(char c, uint8_t colour) {
    uint8_t idx = total_num_char_submitted % HISTORY_SIZE;
    char_buffer_history[idx] = c;
    color_buffer_history[idx] = colour;

    // persist to EEPROM. each slot is only written once per 50 submissions,
    // satisfying the wear-leveling requirement automatically.
    eeprom_save_slot(idx, c, colour, total_num_char_submitted);

    total_num_char_submitted++;
}

// the maximum scroll_offset value. at this offset, the oldest char in the
// buffer reaches the rightmost matrix col. scrolling further would push it
// off-screen and only reveal forgotten chars (empty).
static uint16_t max_scroll_offset(void) {
    if (total_num_char_submitted == 0) return 0;

    // how many chars are actually still in our circular buffer
    uint32_t num_chars_in_buffer =
        (total_num_char_submitted < HISTORY_SIZE) ? total_num_char_submitted : HISTORY_SIZE;

    // matrix col where the newest char's left edge sits at scroll = 0 (rest position)
    int32_t newest_x_no_scroll = MATRIX_NUM_COLUMNS - glyph_width - glyph_width_and_space;

    // matrix col where the oldest visible char's left edge sits at scroll = 0.
    int32_t oldest_x_no_scroll =
        newest_x_no_scroll - (int32_t)(num_chars_in_buffer - 1) * glyph_width_and_space;

    // rightmost matrix col where a char's left edge can sit and still be fully visible
    int32_t right_edge_x = MATRIX_NUM_COLUMNS - glyph_width;

    // max scroll = distance the oldest char needs to travel from its rest
    // position to reach the right edge.
    return (uint16_t)(right_edge_x - oldest_x_no_scroll);
}

// load scrollback from EEPROM into the in memory buffer.
// called once at boot before main_loop starts.
void restore_scrollback_from_eeprom(void) {
    uint32_t max_seq = 0;
    uint8_t any_valid = 0;

    for (uint8_t slot = 0; slot < HISTORY_SIZE; slot++) {
        char c;
        uint8_t colour;
        uint32_t seq;
        eeprom_load_slot(slot, &c, &colour, &seq);

        // EEPROM default state is 0xFF, 0xFFFFFFFF = never written.
        if (seq == 0xFFFFFFFF) continue;

        // sanity check: this slot should contain the char with seq % 50 == slot.
        // if not, EEPROM is corrupted or stale — skip.
        if ((uint8_t)(seq % HISTORY_SIZE) != slot) continue;

        char_buffer_history[slot] = c;
        color_buffer_history[slot] = colour;

        if (!any_valid || seq > max_seq) {
            max_seq = seq;
            any_valid = 1;
        }
    }

    if (any_valid) {
        total_num_char_submitted = max_seq + 1;
    }
}

// replay the restored scrollback to the serial terminal in submission order
// (oldest to newest). each char prints in its stored colour.
// ran after restore_scrollback_from_eeprom in morse.c
void replay_history_to_serial(void) {
    if (total_num_char_submitted == 0) return;

    uint32_t num_chars_in_buffer =
        (total_num_char_submitted < HISTORY_SIZE) ? total_num_char_submitted : HISTORY_SIZE;

    uint32_t oldest_visible_index = total_num_char_submitted - num_chars_in_buffer;

    // print oldest to newest so chars appear left-to-right on the terminal
    for (uint32_t i = 0; i < num_chars_in_buffer; i++) {
        uint32_t char_index = oldest_visible_index + i;
        uint8_t buf_idx = char_index % HISTORY_SIZE;
        // render the chars in memory to serial terminal
        replay_persisted_char_serial(char_buffer_history[buf_idx], color_buffer_history[buf_idx]);
    }
}

// handler to give seven segment initial data
uint32_t get_total_char_submitted(void) { return total_num_char_submitted; }
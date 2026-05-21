#include "serial_output.h"

#include <stdint.h>
#include <stdio.h>

#include "encoding.h"
#include "ledmatrix.h"
#include "terminalio.h"

// wrap to a new line after this many chars on a row. wrap to 78 leave margin
#define MAX_LINE_WIDTH 78

/* Internal Function Declarations */
static void redraw_incomplete(void);
static void print_uppercase(char character);
static void print_finalized_char(char character);

// MODELS
// in-progress char encoding (sentinel-prefixed; 0b1 means "no marks yet")
static uint8_t current_char_encoding = 0b1;
// counter for cursor pos, to handle wrap
static uint8_t cursor_col = 0;

// CONTROLLER
void handle_dot_input_in_serial_output(void) {
    current_char_encoding = (current_char_encoding << 1) | 0b0;
    redraw_incomplete();
}

void handle_dash_input_in_serial_output(void) {
    current_char_encoding = (current_char_encoding << 1) | 1;
    redraw_incomplete();
}

void handle_submit_input_in_serial_output(void) {
    set_display_attribute(FG_GREEN);
    char character = morse_to_char(current_char_encoding);
    print_finalized_char(character);
    current_char_encoding = 0b1;
}

// handle serial char for serial output (interface)
void handle_serial_char_in_serial_output(char character) {
    set_display_attribute(FG_YELLOW);
    print_finalized_char(character);
    current_char_encoding = 0b1;  // erase in progress marks
}

// UTILS
// redraw the in-progress char at the current cursor "anchor".
// prints the char then `\b` so cursor parks back on the char
static void redraw_incomplete(void) {
    set_display_attribute(FG_RED);
    char character = morse_to_char(current_char_encoding);
    print_uppercase(character);  // print and advance terminal cursor
    printf("\b");                // move back cursor (char not completed yet)
}

// print char and inc col and handle wrap at the width limit.
static void print_finalized_char(char character) {
    print_uppercase(character);
    cursor_col++;
    if (cursor_col >= MAX_LINE_WIDTH) {
        printf("\n");  // uart converts \n -> \r\n (start of next line)
        cursor_col = 0;
    }
}

static void print_uppercase(char character) {
    if (character >= 'a' && character <= 'z') {
        character = character - 'a' + 'A';
    }
    printf("%c", character);
}

// replay one restored char during EEPROM scrollback replay at boot.
// called from morse led display
void replay_persisted_char_serial(char c, uint8_t colour) {
    if (colour == COLOUR_GREEN) {
        set_display_attribute(FG_GREEN);
    } else if (colour == COLOUR_YELLOW) {
        set_display_attribute(FG_YELLOW);
    }
    print_finalized_char(c);
}

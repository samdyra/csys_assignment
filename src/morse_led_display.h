#ifndef MORSE_LED_DISPLAY_H_
#define MORSE_LED_DISPLAY_H_

#include <stdint.h>

// event handlers — called by morse.c on input events
void handle_dot_input_led_matrix(void);
void handle_dash_input_led_matrix(void);
void handle_submit_input_led_matrix(void);
void handle_serial_char_led_matrix(char c);

// animation tick — called every iteration of the main loop
void left_shift_led_matrix(void);

// font selection — 0 = small (3-col), 1 = large (5-col). resets scroll.
void set_font(uint8_t font);

// scroll operations — called by joystick input handler
void scroll_one_col_into_past(void);
void scroll_one_col_toward_present(void);
void snap_to_present(void);

// full re-render from the buffer (used after font switch / scroll snap)
void redraw_matrix_from_buffer(void);

void restore_scrollback_from_eeprom(void);

// replay restored scrollback to the serial terminal.
// called once at boot, after clear_terminal().
void replay_history_to_serial(void);

// returns total_num_char_submitted (used to sync the 7-seg after restore).
uint32_t get_total_char_submitted(void);

#endif /* MORSE_LED_DISPLAY_H_ */
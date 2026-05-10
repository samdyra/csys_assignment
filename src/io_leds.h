#ifndef IO_LEDS_H_
#define IO_LEDS_H_

#include <stdint.h>

void initialize_uq_io_board_led(void);
void render_uq_io_board_led(void);

void handle_dot_input_in_uq_io_led(uint8_t has_mark_in_current_char);
void handle_dash_input_in_uq_io_led(uint8_t has_mark_in_current_char);
void handle_submit_input_in_uq_io_led(uint8_t consecutive_submits);

#endif
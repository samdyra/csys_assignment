#ifndef SEVEN_SEGMENT_H_
#define SEVEN_SEGMENT_H_

#include <stdint.h>

/* Initialise pin directions and disable JTAG. Call once at startup. */
void seven_segment_init(void);

/* Render one digit and switch to the other for next call.
 * Designed to be called from the ~2ms timer ISR.
 * mark_count: number of marks in in-progress char (0=off, 10+=dash)
 * submitted_count: total chars submitted (mod 16, hex on left digit)
 */
void seven_segment_step(uint8_t mark_count, uint8_t submitted_count);

void render_seven_segment(void);
void handle_dot_input_in_seven_segment(void);
void handle_dash_input_in_seven_segment(void);
void handle_submit_input_in_seven_segment(void);
void handle_serial_char_in_seven_segment(void);

#endif
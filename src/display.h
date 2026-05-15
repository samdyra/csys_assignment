/*
 * display.h
 *
 * Author: Ryan Wang
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#define GLYPH_WIDTH_SMALL 3
#define GLYPH_WIDTH_LARGE 5

#include <stdint.h>

/*
 * display a start screen
 */
void start_splash_display(void);

/*
 * draws a small char glyph on the LED matrix starting at x_position (columnn number)
 */
void draw_small_char(char character, uint8_t x_position, uint8_t colour);

/*
 * draws a big char glyph on the LED matrix starting at x_position (columnn number)
 */
void draw_large_char(char character, uint8_t x_position, uint8_t colour);

// draws a single column of a small glyph onto a single column of the LED matrix.
void draw_small_glyph_column(char c, uint8_t col_in_glyph, uint8_t colour, uint8_t matrix_col);

// draws a single column of a large glyph onto a single column of the LED matrix.
void draw_large_glyph_column(char c, uint8_t col_in_glyph, uint8_t colour, uint8_t matrix_col);

#endif

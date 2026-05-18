/*
 * ledmatrix.h
 *
 * Author: Peter Sutton, Ryan Wang
 */

#ifndef LEDMATRIX_H_
#define LEDMATRIX_H_

#include <stdint.h>

#include "spi.h"

// matrix dimensions
#define MATRIX_NUM_COLUMNS 16
#define MATRIX_NUM_ROWS 8

// colour definitions (upper nibble = green intensity, lower = red intensity)
#define COLOUR_BLACK 0x00
#define COLOUR_GREEN 0xF0
#define COLOUR_RED 0x0F
#define COLOUR_YELLOW 0xFF

// shift directions for ledmatrix_shift_display (per LED matrix spec: 0000UDLR)
#define SHIFT_UP 0x08
#define SHIFT_DOWN 0x04
#define SHIFT_LEFT 0x02
#define SHIFT_RIGHT 0x01

// hardware primitives
void ledmatrix_update_column(uint8_t x, uint8_t pixels[MATRIX_NUM_ROWS]);
void ledmatrix_clear(void);
void ledmatrix_shift_display(uint8_t direction);

#endif /* LEDMATRIX_H_ */
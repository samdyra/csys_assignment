/*
 * ledmatrix.c
 *
 * Author: Peter Sutton
 *
 * See the LED matrix Reference for details of the SPI commands used.
 */

#include "ledmatrix.h"

#include <avr/io.h>
#include <stdint.h>

#include "morse_led_display.h"
#include "spi.h"

#define CMD_UPDATE_COL 0x03
#define CMD_CLEAR_SCREEN 0x0F
#define CMD_SHIFT_DISPLAY 0x04

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
    (void)spi_send_byte(CMD_SHIFT_DISPLAY);
    (void)spi_send_byte(direction);
}
/*
 * terminalio.c
 *
 * Author: Peter Sutton
 */

#include "terminalio.h"

#include <stdint.h>
#include <stdio.h>

void move_terminal_cursor(int x, int y) { printf("\x1b[%d;%dH", y, x); }

void clear_terminal(void) { printf("\x1b[2J"); }

// emit the ANSI SGR escape "ESC [ N m" - N is the parameter (e.g. FG_RED = 31).
void set_display_attribute(DisplayParameter parameter) {
    printf("\x1b[%dm", parameter);
}  // \x1b = ESC, m = graphic rendition
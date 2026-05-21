#include "input.h"

#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>

#include "adc.h"
#include "ledmatrix.h"
#include "morse_led_display.h"
#include "serialio.h"

/* Internal Function Declarations */
static void handle_press_button_async(void);
static void handle_press_button_sync(void);

// Access morse.c's timer counter
extern volatile uint32_t shared_counter_0;

// event handlers provided by morse.c
extern void handle_dot(void);
extern void handle_dash(void);
extern void handle_submit(void);
extern void handle_char_from_serial(char c);

// Sync mode duration thresholds (ms)
#define SYNC_DOT_DASH_THRESHOLD 200
#define SYNC_FIRST_SUBMIT_AT 1000
#define SYNC_SECOND_SUBMIT_AT 2000

// Event listeners
// read S0 to determine sync async mode
void listen_button_input(void) {
    // use sync mode if S0 is high
    if (PINA & (1 << PINA4)) {
        handle_press_button_sync();
    } else {
        handle_press_button_async();
    }
}

void listen_font_switch_input(void) {
    static uint8_t last_s1 = 0;
    uint8_t curr_s1 = (PINA & (1 << PINA5)) ? 1 : 0;
    if (curr_s1 != last_s1) {
        set_font(curr_s1);
        redraw_matrix_from_buffer();
        last_s1 = curr_s1;
    }
}

void listen_serial_input(void) {
    if (!serial_input_available()) return;

    int c_int = fgetc(stdin);              // 0-255 or EOF (-1)
    handle_char_from_serial((char)c_int);  // safe to cast since we already checked availability
}

void listen_joystick_input(void) {
    static uint32_t last_scroll_tick = 0;

    uint8_t x = adc_read_channel_8bit(6);  // PA6 = joystick X

    // deadzone around center (~128). ignore small tilts.
    if (x >= 104 && x <= 152) return;

    // determine direction and magnitude from center
    uint8_t magnitude;
    int8_t direction;  // +1 = into past (tilt right), -1 = toward present (tilt left)
    if (x < 128) {
        direction = -1;
        magnitude = 128 - x;
    } else {
        direction = +1;
        magnitude = x - 128;
    }

    // scroll interval depends on tilt magnitude (slight tilt = slow, full tilt = fast)
    uint32_t interval_ms;
    if (magnitude < 48) {
        interval_ms = 200;  // slow
    } else if (magnitude < 96) {
        interval_ms = 100;  // medium
    } else {
        interval_ms = 50;  // fast
    }

    if (shared_counter_0 - last_scroll_tick < interval_ms) return;  // rate limiter/stopper
    last_scroll_tick = shared_counter_0;

    if (direction > 0) {
        scroll_one_col_into_past();
    } else {
        scroll_one_col_toward_present();
    }
}

// CONTROLLER
// async mode: B0, B1, B2
static void handle_press_button_async(void) {
    static uint8_t prev = 0b000;
    uint8_t curr = PINB & ((1 << PINB2) | (1 << PINB1) | (1 << PINB0));
    uint8_t edges = curr & ~prev;
    prev = curr;

    // dot/dash/submit handlers handled in morse.c
    if (edges & (1 << 0)) handle_dot();
    if (edges & (1 << 1)) handle_dash();
    if (edges & (1 << 2)) handle_submit();
}

// sync mode: B0 only
static void handle_press_button_sync(void) {
    static uint8_t was_pressed = 0;
    static uint32_t press_started = 0;
    static uint32_t release_started = 0;
    // start at 2 (capped), no submits fired until user has pressed at least once.
    static uint8_t submits_fired = 2;  // On first press we'll reset this to 0.

    uint8_t is_pressed = (PINB & (1 << PINB0)) ? 1 : 0;
    uint32_t current_time = shared_counter_0;

    // rising edge: user just press (not released yet)
    if (!was_pressed && is_pressed) {
        press_started = current_time;
        submits_fired = 0;  // a press cancels any pending submit timers
    }

    // falling edge: user release press (define dot or dash from press duration)
    if (was_pressed && !is_pressed) {
        uint32_t held_for = current_time - press_started;
        if (held_for < SYNC_DOT_DASH_THRESHOLD) {
            handle_dot();
        } else {
            handle_dash();
        }
        release_started = current_time;
    }

    was_pressed = is_pressed;

    // while released (do nothing), watch for submit thresholds
    if (!is_pressed && submits_fired < 2) {
        uint32_t since_release = current_time - release_started;
        if (submits_fired == 0 && since_release >= SYNC_FIRST_SUBMIT_AT) {
            handle_submit();
            submits_fired = 1;
        } else if (submits_fired == 1 && since_release >= SYNC_SECOND_SUBMIT_AT) {
            handle_submit();
            submits_fired = 2;
        }
    }
}

// utils
void input_init(void) {
    // B0, B1, B2 as inputs
    DDRB &= ~((1 << DDB0) | (1 << DDB1) | (1 << DDB2));

    // S0 (PA4) elects async vs sync mode
    DDRA &= ~(1 << DDA4);
    // S1 font selection
    DDRA &= ~(1 << DDA5);

    // scroll init
    DDRA &= ~(1 << DDA6);
    adc_init();
}

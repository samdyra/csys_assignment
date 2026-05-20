#include "io_leds.h"

#include <avr/io.h>
#include <stdint.h>

/* Internal Function Declarations */
static void update_io_leds(void);
static void enqueue_tick(uint8_t tick);
static uint8_t dequeue_tick(void);

// MODELS
// access morse.c's timer counter
extern volatile uint32_t shared_counter_0;
/* IO LED ANIMATION QUEUE */
#define TICK_QUEUE_SIZE 32
static uint8_t tick_queue[TICK_QUEUE_SIZE];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;
static uint8_t queue_count = 0;
// store last time io led tick
static uint32_t last_time_io_led_tick = 0;
// Stored led pattern
static uint8_t led_pattern = 0x00;

// VIEWS
void render_uq_io_board_led(void) {
    if (shared_counter_0 - last_time_io_led_tick >= 100) {
        update_io_leds();
    }
}

// CONTROLLERS
static void update_io_leds(void) {
    if (queue_count == 0) return;  // nothing to animate

    uint8_t tick = dequeue_tick();
    led_pattern = (led_pattern << 1) | tick;
    // port A = lower half of led
    PORTA = (PORTA & 0xF0) | (led_pattern & 0x0F);
    // port B = uppper half of led
    PORTD = (PORTD & 0b11000011) | ((led_pattern & 0xF0) >> 2);

    last_time_io_led_tick = shared_counter_0;
}

void handle_dot_input_in_uq_io_led(uint8_t has_mark_in_current_char) {
    // dot input in UQ IO LED
    if (has_mark_in_current_char) {
        enqueue_tick(0);  // inter-mark gap
        enqueue_tick(1);
    } else {
        enqueue_tick(1);
    }

    update_io_leds();
}

void handle_dash_input_in_uq_io_led(uint8_t has_mark_in_current_char) {
    // dash input in UQ IO LED
    if (has_mark_in_current_char) {
        enqueue_tick(0);  // inter-mark gap
        enqueue_tick(1);
        enqueue_tick(1);
        enqueue_tick(1);
    } else {
        enqueue_tick(1);
        enqueue_tick(1);
        enqueue_tick(1);
    }

    update_io_leds();
}

void handle_submit_input_in_uq_io_led(uint8_t consecutive_submits) {
    // submit input in UQ IO LED
    // (early return for >1 is handled in morse.c)

    if (consecutive_submits == 0) {
        enqueue_tick(0);
        enqueue_tick(0);
        enqueue_tick(0);
    } else if (consecutive_submits == 1) {
        enqueue_tick(0);
        enqueue_tick(0);
    }

    update_io_leds();
}

void handle_serial_char_in_uq_io_led(uint8_t encoding) {
    uint8_t mask = 0b10000000;  // start with mask = 0b10000000 (bit 7)
    // loop through mask, until find 1
    while (mask && !(encoding & mask)) {  // encoding ex: 0b0010010
        mask = mask >> 1;                 // shift right until we hit a 1 or mask = 0
    }

    if (mask == 0) return;  // if nothing was set (all 0), do nothing

    // mark_mask (single-bit cursor): it starts one bit below the prefix
    // and walks right
    uint8_t mark_mask = mask >> 1;

    // iterate actual data (a bit after the first 1)
    uint8_t is_first = 1;
    while (mark_mask) {
        if (!is_first) {
            enqueue_tick(0);  // inter-mark gap
        }

        if (encoding & mark_mask) {
            // 1 = dash = 3 high beats
            enqueue_tick(1);
            enqueue_tick(1);
            enqueue_tick(1);
        } else {
            // dot: 1 ON beat
            enqueue_tick(1);
        }

        is_first = 0;
        mark_mask = mark_mask >> 1;
    }

    update_io_leds();  // immediate first beat
}

// UTILS

// init uq io board
void initialize_uq_io_board_led(void) {
    DDRA |= (1 << DDRA0) | (1 << DDRA1) | (1 << DDRA2) |
            (1 << DDRA3);  // set a0, a1, a2, a3 to be outputs
    DDRD |= (1 << DDRD5) | (1 << DDRD4) | (1 << DDRD3) |
            (1 << DDRD2);  // set d3, d4, d5, d2 to be outputs
}

static void enqueue_tick(uint8_t tick) {
    if (queue_count < TICK_QUEUE_SIZE) {
        // write the new tick at the tail position.
        // tick & 1 ensures only store 1 bit 0 or 1, never accidentally larger
        tick_queue[queue_tail] = tick & 1;

        // make the array circular
        // advance tail by 1, wrapping back to 0 if it would go past the end.
        //  ex: queue_tail = 30 → (30+1) % 32 = 31    (normal step)
        // exL  queue_tail = 31 → (31+1) % 32 = 0     (wraps to start)
        queue_tail = (queue_tail + 1) % TICK_QUEUE_SIZE;

        queue_count++;
    }
}

static uint8_t dequeue_tick(void) {
    // Read the oldest tick (head).
    uint8_t tick = tick_queue[queue_head];

    // Advance head by 1, wrapping back to 0 the same way as tail.
    // head will chase tail, the range of head and tail = queue count
    queue_head = (queue_head + 1) % TICK_QUEUE_SIZE;

    queue_count--;
    return tick;
}
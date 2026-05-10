#include "seven_segment.h"

#include <avr/io.h>
#include <stdint.h>

/* Segment bit positions per the wiring */
#define SEG_A (1 << 0)
#define SEG_B (1 << 1)
#define SEG_C (1 << 2)
#define SEG_D (1 << 3)
#define SEG_E (1 << 4)
#define SEG_F (1 << 5)
#define SEG_G (1 << 6)
#define SEG_DP (1 << 7)

// MODELS
// marks in in progress char
static volatile uint8_t seven_seg_mark_count = 0;
// total chars submitted
static volatile uint8_t seven_seg_submitted_count = 0;

// VIEWS
void render_seven_segment(void) {
    seven_segment_step(seven_seg_mark_count, seven_seg_submitted_count);
}

// CONTROLLERS
void handle_dot_input_in_seven_segment(void) { seven_seg_mark_count++; }
void handle_dash_input_in_seven_segment(void) { seven_seg_mark_count++; }
void handle_submit_input_in_seven_segment(void) {
    seven_seg_submitted_count++;
    seven_seg_mark_count = 0;
}

// UTILS
static const uint8_t seg_patterns[16] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
    SEG_B | SEG_C,                                          // 1
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                  // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                  // 3
    SEG_B | SEG_C | SEG_F | SEG_G,                          // 4
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                  // 5
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,          // 6
    SEG_A | SEG_B | SEG_C,                                  // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          // 9
    SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,          // A
    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,                  // b
    SEG_A | SEG_D | SEG_E | SEG_F,                          // C
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,                  // d
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,                  // E
    SEG_A | SEG_E | SEG_F | SEG_G,                          // F
};

void seven_segment_init(void) {
    DDRC = 0xFF;
    DDRD |= (1 << DDRD7);  // cc
}

void seven_segment_step(uint8_t mark_count, uint8_t submitted_count) {
    static uint8_t current_digit = 0;
    uint8_t segments;

    PORTC = 0;  // blank to prevent ghosting

    if (current_digit == 0) {
        // right: mark count
        if (mark_count == 0) {
            segments = 0;
        } else if (mark_count > 9) {
            segments = SEG_G;  // "-"
        } else {
            segments = seg_patterns[mark_count];
        }
        PORTD &= ~(1 << PORTD7);  // clear cc
        PORTC = segments;
        current_digit = 1;
    } else {
        // left: submitted count mod 16,
        segments = seg_patterns[submitted_count & 0x0F];
        if (mark_count > 0) {
            segments |= SEG_DP;  // DP on if right is on
        }
        PORTD |= (1 << PORTD7);  // light cc
        PORTC = segments;
        current_digit = 0;
    }
}
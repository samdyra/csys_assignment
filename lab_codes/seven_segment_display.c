#include <avr/io.h>

/* Seven segment display values */
uint8_t seven_seg[16] = {63,         6,          91,         79,        102,        109,
                         125,        7,          127,        111,       0b01110111, 0b01111100,
                         0b00111001, 0b01011110, 0b01111001, 0b01110001};

int seven_segment(void) {
    uint8_t digit;

    /* Set port A pins to be outputs, port C pins to be inputs */
    DDRA = 0xFF;
    DDRC = 0; /* This is the default, could omit. */
    while (1) {
        /* Read in a digit from lower half of port C pins */
        /* We read the whole byte and mask out upper bits */
        digit = PINC & 0x0F;
        /* Write out seven segment display value to port A */
        if (digit < 16) {
            PORTA = seven_seg[digit];
        } else {
            PORTA = 0;
        }
    }
}
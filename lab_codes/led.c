/*
 * lab16-2.c
 *
 * LED Matrix SPI example
 *
 * Replace the "<-YOUR CODE HERE->" comments with your code.
 */

#include <avr/io.h>

/* Function prototypes */
void spi_setup_master(void);
uint8_t spi_send_byte(uint8_t byte);

int led(void) {
    TCCR1B = (1 << CS12) | (1 << CS10);
    spi_setup_master();

    spi_send_byte(0x0F); /* Clear screen */
    TCNT1 = 0;
    while (TCNT1 < 7811) {
    }  // do nothing for 1 second

    spi_send_byte(0x10); /* Test pattern */
    TCNT1 = 0;
    while (TCNT1 < 15624) {
    }  // do nothing for 2 seconds

    spi_send_byte(0x00); /* Clear screen */
    TCNT1 = 0;
    /* This loop will fade through the cycle
     * 0) black -> 1) red -> 2) yellow -> 3) green -> 0) black ...
     */
    while (1) {
        uint8_t step = (TCNT1 >> 8) & 0x0F;   // get bits 11, 10, 9 & 8 from TCNT1 (value 0 to 15)
        uint8_t mode = (TCNT1 >> 12) & 0x03;  // get bits 13 & 12 from TCNT1 (value 0 to 3)
        uint8_t colour;
        switch (mode) {
            case 0:
                // fade from black to red
                // step = 0 is black (no green, no red), so 0x00
                // step = 15 is red (no green, full red), so 0x0F
                // thus, green is always zero, red is step value
                colour = step;
                break;
            case 1:
                // fade from red to yellow
                // step = 0 is red (no green, full red), so 0x0F
                // step = 15 is yellow (full green, full red), so 0xFF
                // thus, green is step value, red is always 15
                colour = (step << 4) | 0x0F;
                break;
            case 2:
                // fade from yellow to green
                // step = 0 is yellow (full green, full red), so 0xFF
                // step = 15 is green (full green, no red), so 0xF0
                // thus, green is always 15, red is 15-step
                colour = 0xF0 | (15 - step);
                // or: colour = ~step; prove this
                break;
            case 3:
            default:  // need a default here otherwise the compiler will complain
                // fade from green to black
                // step = 0 is green (full green, no red), so 0xF0
                // step = 15 is black (no green, no red), so 0x00
                // thus, green is 15-step, red is always zero
                colour = 15 - step;
                break;
        }
        spi_send_byte(0x00);  // send "colour every pixel" command byte
        for (uint8_t k = 0; k < 128; k++) {
            // send the colour byte 128 times i.e. once for each pixel
            // spi_send_byte(colour);
            spi_send_byte(((k ^ (k >> 4)) & 1) ? colour : ~colour);
        }
    }
}

void spi_setup_master(void) {
    // Set up SPI communication as a master
    // Make the SS, MOSI and SCK pins outputs. These are pins
    // 4, 5 and 7 of port B on the ATmega324A
    DDRB |= ((1 << DDRB4) | (1 << DDRB5) | (1 << DDRB7));

    // Set the slave select (SS) line high
    // The LED matrix will ignore anything happening on the SS, MOSI and SCK pins for now
    // You want to do this because while the SPI is getting set up,
    // the output to these pins can be unpredictable
    PORTB |= (1 << PORTB4);

    // Set up the SPI control registers SPCR0 and SPSR0.
    // Remember that the datasheet doesn't have the trailing "0", but your C code will require
    // it. We want:
    // - No SPI interrupt;
    // - SPI enabled;
    // - MSB trasmitted first;
    // - The AVR in master mode;
    // - The leading edge to be rising and sampling; and
    // - A clock divider of 128.
    SPCR0 = (0 << SPIE0) | (1 << SPE0) | (0 << DORD0) | (1 << MSTR0) | (0 << CPOL0) | (0 << CPHA0) |
            (1 << SPR00) | (1 << SPR10);
    SPSR0 = (0 << SPI2X0);

    // Take SS (slave select) line low
    // Now the LED matrix will receive the SPI signals
    PORTB &= ~(1 << PORTB4);  // set bit 4 of PORTB to 0
}

uint8_t spi_send_byte(uint8_t byte) {
    // Write out the byte to the SPDR0 register. This will initiate the transfer. We then  wait
    // until the most significant bit of SPSR0 (SPIF0 bit) is set - this indicates that the transfer
    // is complete. The final read of SPSR0 followed by a read of SPDR0 will cause the SPIF0 bit to
    // be reset to 0. See page 173 of the ATmega324A datasheet (2018 version).
    SPDR0 = byte;
    while (!(SPSR0 & (1 << SPIF0))) {
        ;
    }
    return SPDR0;
}

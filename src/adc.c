#include "adc.h"

#include <avr/io.h>

void adc_init(void) {
    // AVCC as reference, left-adjust result (so 8 MSBs land in ADCH)
    ADMUX = (1 << REFS0) | (1 << ADLAR);
    // Enable ADC, prescaler 64 → ADC clock = 125 kHz at 8 MHz CPU
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint8_t adc_read_channel_8bit(uint8_t channel) {
    // Select channel (preserve REFS and ADLAR bits in upper part of ADMUX)
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);
    // Start conversion
    ADCSRA |= (1 << ADSC);
    // Wait for conversion to finish (~104us at 125kHz — short busy-wait, not a delay)
    while (ADCSRA & (1 << ADSC));
    // Return 8 MSBs of the 10-bit result
    return ADCH;
}
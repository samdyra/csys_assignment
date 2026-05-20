#include "adc.h"

#include <avr/io.h>

void adc_init(void) {
    // res0 set avcc as reference, adlar for left-adjust result (so 8 MSBs land in ADCH)
    ADMUX = (1 << REFS0) | (1 << ADLAR);
    // aden rnable ADC,  adps for prescaler 64 , ADC clock = 125 kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint8_t adc_read_channel_8bit(uint8_t channel) {
    // Set channel (low 5 bits), preserving REFS/ADLAR config
    // (high 3 bits). 0xE0 keeps config, 0x1F masks channel to its 5 bit field.
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);
    // start conversion
    ADCSRA |= (1 << ADSC);
    // wait for conversion to finish
    while (ADCSRA & (1 << ADSC));
    // return 8 MSBs of the 10-bit result
    return ADCH;
}
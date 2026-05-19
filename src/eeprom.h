#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>

// write one scrollback slot to EEPROM.
// slot must be 0..49. seq should be the submission number.
void eeprom_save_slot(uint8_t slot, char c, uint8_t colour, uint32_t seq);

// read one scrollback slot from EEPROM.
// seq_out == 0xFFFFFFFF means uninitialized (EEPROM default state).
void eeprom_load_slot(uint8_t slot, char* c_out, uint8_t* colour_out, uint32_t* seq_out);

#endif /* EEPROM_H_ */
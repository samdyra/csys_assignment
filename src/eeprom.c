#include "eeprom.h"

#include <avr/eeprom.h>
#include <stdint.h>

// https://onlinedocs.microchip.com/oxy/GUID-317042D4-BCCE-4065-BB05-AC4312DBC2C4-en-US-2/GUID-735C4A48-0970-4086-A5CA-89AC469956ED.html

// per-slot layout in EEPROM.
// __attribute__((packed)) is a GCC-specific instruction that disables
// the compiler's alignment padding. by default the compiler might insert invisible bytes after
// `colour` so that `seq` starts on a 4-byte boundary that would make the struct 8 bytes instead of
// 6: the layout is predictable across compilers / build flags
struct eeprom_slot {
    uint8_t c;       // the char. use uint8_t instead of char because `char` signedness is
                     // implementation-defined;
    uint8_t colour;  // the LED matrix colour stored alongside the char
    uint32_t seq;    // sequence number = submission count at the time
                     // this slot was written. used on boot to find the
                     // newest slot (max seq) and reconstruct
                     // total_num_char_submitted.
} __attribute__((packed));

// reserve 50 slots in the .eeprom linker section.
//
// EEMEM is a macro from <avr/eeprom.h> defined as:
//   #define EEMEM __attribute__((section(".eeprom")))
// set the linker put this variable in the .eeprom section not RAM.
static struct eeprom_slot EEMEM eeprom_slots[50];

// write one slot to EEPROM.
// the AVR EEPROM API takes POINTERS, not raw integer addresses. so we
// pass the address of the specific field
// (use update instead of write for efficiency)
void eeprom_save_slot(uint8_t slot, char c, uint8_t colour, uint32_t seq) {
    // eeprom_update_XXXX( pointer_to_where,  value_to_write ); use byte (8 bit)
    eeprom_update_byte(&eeprom_slots[slot].c, (uint8_t)c);
    eeprom_update_byte(&eeprom_slots[slot].colour, colour);
    // At slot number slot in the EEPROM array, take the address of its seq field, and write the
    // value of the seq parameter into it. use dword (32bit)
    eeprom_update_dword(&eeprom_slots[slot].seq, seq);
}

// read one slot from EEPROM.
// return as pointer * in param
// use * to dereference (assign the value to the given param)
void eeprom_load_slot(uint8_t slot, char* c_out, uint8_t* colour_out, uint32_t* seq_out) {
    *c_out = (char)eeprom_read_byte(&eeprom_slots[slot].c);
    *colour_out = eeprom_read_byte(&eeprom_slots[slot].colour);
    *seq_out = eeprom_read_dword(&eeprom_slots[slot].seq);
}
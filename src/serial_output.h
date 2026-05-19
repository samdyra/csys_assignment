#include <stdint.h>

#ifndef SERIAL_OUTPUT_H_
#define SERIAL_OUTPUT_H_

// dot pressed: update the in progress char display
void handle_dot_input_in_serial_output(void);

// dash pressed: update the in progress char display
void handle_dash_input_in_serial_output(void);

// submit pressed: fix the in progress char (or print a space if no marks)
void handle_submit_input_in_serial_output(void);

// character arrived from serial input: display it (overwriting any incomplete)
void handle_serial_char_in_serial_output(char c);

// print a single persisted char during EEPROM replay on boot.
void replay_persisted_char_serial(char c, uint8_t colour);

#endif /* SERIAL_OUTPUT_H_ */
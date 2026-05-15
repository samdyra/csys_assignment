#ifndef INPUT_H_
#define INPUT_H_

void input_init(void);
void input_poll(void);
void listen_serial_input(void);
void listen_font_switch_input(void);
void listen_joystick_input(void);

#endif
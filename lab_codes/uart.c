/*
 * FILE: lab16-1.c
 *
 * Replace the "<-YOUR CODE HERE->" comments with your code.
 */

#include <avr/interrupt.h>
#include <avr/io.h>

/*
 * main -- Main program.
 */
int uart(void) {
    /* Set the baud rate to 19200 */
    UBRR0 = 25;

    /*
    ** Enable transmission and receiving via UART and also
    ** enable the Receive Complete Interrupt.
    ** (See pages 194-195 of the datasheet)
    */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    /* NOTE: No need to set UCSR0C - we just want the default value */

    /* Enable interrupts */
    sei();

    /* Sit back and let it happen - this will loop forever */
    for (;;) {
    }
}

/*
 * Define the interrupt handler for UART Receive Complete - i.e. a new
 * character has arrived in the UART Data Register (UDR).
 */
ISR(USART0_RX_vect) {
    /* A character has been received - we will read it. If it is
    ** lower case, we will convert it to upper case and send it
    ** back, otherwise we just send it back as is
    */
    char input;

    /* Extract character from UART Data register and place in input
    ** variable
    */
    input = UDR0;

    /* Convert character to upper case if it is lower case */
    /* <- YOUR CODE HERE -> */

    /* Send the character back over the serial connection */
    UDR0 = input;
}

// /*
//  * FILE: lab12-1.c
//  *
//  * Replace the "<-YOUR CODE HERE->" comment lines with your code.
//  */

// #include <avr/io.h>

// /*
//  * main -- Main program
//  */
// int counter(void) {
//     /* Set OC1A pin to be an output */
//     DDRD |= (1 << DDRD5); /* ddrd5 is the pin for oc1a, so we set it to 1 to make it an output */

//     /* Set output compare register value  (this is the value that the timer counter will be
//     compared
//      * to) */
//     OCR1A = 15624;

//     /* Set timer counter control registers A and B so that
//      *  - mode is - clear counter on compare match
//      *  - output compare match action is to toggle pin OC1A
//      *  - correct clock prescale value is chosen.
//      * TCCR1C can just stay as default value (0).
//      */
//     TCCR1A = (1 << COM1A0); /* com1a0 is used so that its on toggle mode, also make the pin 5d as
//     compare timer output instead of normal pin */
//     TCCR1B = (1 << WGM12)   /* wgm12 is for ctc mode */
//              | (1 << CS11) |
//              (1 << CS10); /* cs is for the prescaler (011), therefore the cs12 is 0) */

//     /* Do nothing forever - the hardware takes care of everything */
//     while (1) {
//         ;
//     }
// }

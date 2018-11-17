/*
 * ----------------------------------------------------------------------------
 * asyncronous serial communication interface functions
 *
 * Author       : Jungho Moon
 * Target MCU   : ATMEL AVR ATmega64/128
 * ----------------------------------------------------------------------------
 */

#define UART_GLOBALS

#include "bsp.h"
#include "uart.h"

/* ----------------------------------------------------------------------------
 * initialize UART
 * -------------------------------------------------------------------------- */
void uart_init(void)
{
    #ifdef URSEL
    _UCSRC = (1<<URSEL)| (3<<UCSZ0);        // asyn, no parity, 1 stop bit, 8 data bits
    #else
    _UCSRC = (3<<UCSZ00);                   // asyn, no parity, 1 stop bit, 8 data bits
    #endif

    _UBRRH = 0;
    _UBRRL = (OSC_FREQUENCY/8.)/BAUD_RATE - 0.5;

    _UCSRB = _BV(RXEN) | _BV(TXEN);
    _UCSRA |= _BV(U2X);                     // enable U2X mode
}

/* ----------------------------------------------------------------------------
 * check if a new byte has been received
 * return value
 *  - 1 if received, 0 otherwise
 * -------------------------------------------------------------------------- */
uint8_t inline uart_receive_check(void)
{
    if(_UCSRA & (1<<RXC))
        return 1;
    else 
        return 0;
}

/* ----------------------------------------------------------------------------
 * read a byte from UART
 * return value
 *  - received data of 1 byte
 * -------------------------------------------------------------------------- */
int8_t uart_getch(void)
{
    while (!(_UCSRA & (1<<RXC))) ;          // wait for data to be received

    return _UDR;
}

/* ----------------------------------------------------------------------------
 * write a byte to UART
 * arguments
 *  - ch: 1-byte data to send
 * -------------------------------------------------------------------------- */
void uart_putch(int8_t ch)
{
    while(!(_UCSRA & (1<<UDRE))) ;          // wait for empty transmit buffer

    _UDR = ch;
}

/* ----------------------------------------------------------------------------
 * write a string to UART
 * arguments
 *  - str: pointer to string to send
 * -------------------------------------------------------------------------- */
void uart_puts(int8_t* str)
{
    int8_t ch;

    while((ch = *str++))
        uart_putch(ch);
}

/* ----------------------------------------------------------------------------
 * write ascii values of a 1-byte data to UART
 * arguments
 *  - ch: 1-byte data the ascii values of which will be sent
 * example
 * uart_print_hex8(0x3A) sends '3' and 'A' to UART
 * -------------------------------------------------------------------------- */
void uart_print_hex8(int8_t ch)
{
    uint8_t low, high;

    high = (ch>>4)&0x0f;
    low = ch & 0x0f;

    if(high >= 10)
        uart_putch(high + 'A' - 10);
    else
        uart_putch(high + '0');

    if(low >= 10)
        uart_putch(low + 'A' - 10);
    else
        uart_putch(low + '0');
}

/* ----------------------------------------------------------------------------
 * write ascii values of a 2-byte data to UART
 * arguments
 *  - ch: 2-byte data the ascii values of which will be sent
 * example
 * uart_print_hex16(0x3A27) sends '3', 'A', '2', and '7' to UART
 * -------------------------------------------------------------------------- */
void uart_print_hex16(int16_t ch)
{
    uart_print_hex8(ch>>8);
    uart_print_hex8(ch&0xff);
}

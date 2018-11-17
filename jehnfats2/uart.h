/*
 * ----------------------------------------------------------------------------
 *          Header file for UART interface functions
 *                              written by Jungho Moon
 * ----------------------------------------------------------------------------
 */

#ifndef UART_H
#define UART_H

#ifdef  UART_GLOBALS
#define UART_EXT
#else
#define UART_EXT extern
#endif

#define BAUD_RATE           115200L         // Baud rate
#define UART                0               // select UART0
//#define UART                1               // select UART1
//#define UART                2               // select UART2
//#define UART                                // select this when there is only one UART (ATMega8, ATMega16, ATMega32)

// UART registers defenition
#if(UART == 0)
    #define  _UCSRA         UCSR0A
    #define  _UCSRB         UCSR0B
    #define  _UCSRC         UCSR0C
    #define  _UBRRL         UBRR0L
    #define  _UBRRH         UBRR0H
    #define  _UDR           UDR0
#elif(UART == 1)
    #define  _UCSRA         UCSR1A
    #define  _UCSRB         UCSR1B
    #define  _UCSRC         UCSR1C
    #define  _UBRRL         UBRR1L
    #define  _UBRRH         UBRR1H
    #define  _UDR           UDR1
#elif(UART == 2)
    #define  _UCSRA         UCSR2A
    #define  _UCSRB         UCSR2B
    #define  _UCSRC         UCSR2C
    #define  _UBRRL         UBRR2L
    #define  _UBRRH         UBRR2H
    #define  _UDR           UDR2
#elif(defined UART)
    #define  _UCSRA         UCSRA
    #define  _UCSRB         UCSRB
    #define  _UCSRC         UCSRC
    #define  _UBRRL         UBRRL
    #define  _UBRRH         UBRRH
    #define  _UDR           UDR
#endif

// function prototypes
void uart_init(void);
uint8_t uart_receive_check(void);
int8_t uart_getch(void);
void uart_putch(int8_t);
void uart_puts(int8_t *);
void uart_print_hex8(int8_t);
void uart_print_hex16(int16_t);
#endif

/*
 * ----------------------------------------------------------------------------
 * SPI functions
 *
 * Author       : Jungho Moon
 * Target MCU   : ATMEL ATmega64/128
 * ----------------------------------------------------------------------------
 */

#define SPI_GLOBALS

#include "bsp.h"
#include "spi.h"

/* ----------------------------------------------------------------------------
 * initialize the SPI port as master
 * -------------------------------------------------------------------------- */
void spi_master_init(void)
{
    // configure SPI I/O pints
    PORTB |= _BV(SCLK);                     // set SCLK to 'H'
    DDRB |= _BV(SCLK) | _BV(MOSI);          // configure SS, SCK, MOSI as output
    DDRB &= ~_BV(MISO);                     // configure MISO as input

    // set SPI bitrate (bit rate = fosc/2)
    // set clock polarity, and phase (mode: 0)
    SPCR = _BV(SPE) | _BV(MSTR);            // master mode with clock freq = fosc/4
    SPSR = _BV(SPI2X);                      // set double SPI speed bit (fsck = fosc/2)
}

/* ----------------------------------------------------------------------------
 * initialize the SPI port as slave
 * -------------------------------------------------------------------------- */
void spi_slave_init(void)
{
    // setup SPI I/O pints
    DDRB |= _BV(MISO);                      // configure MISO as output

    // set clock polarity, and phase (mode: 0)
    SPCR = _BV(SPE);                        // slave mode
}

/* ----------------------------------------------------------------------------
 * write and read 1-byte value through the SPI interface
 * arguments
 *  - value: 1-byte data to send
 * return value
 *  - received data of 1 byte
 * -------------------------------------------------------------------------- */
uint8_t spi_read_write_byte(uint8_t data)
{
    SPDR = data;
    while(!(SPSR & _BV(SPIF))) ;

    return SPDR;
}


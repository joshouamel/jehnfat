/*
 * ----------------------------------------------------------------------------
 *          Header file for SPI interface functions
 *                              written by Jungho Moon
 * ----------------------------------------------------------------------------
 */
 
#ifndef SPI_H
#define SPI_H

// function prototypes
void spi_master_init(void);
void spi_slave_init(void);
uint8_t spi_read_write_byte(uint8_t);
#endif

/*
 * ----------------------------------------------------------------------------
 *          Definitions for fat32 demo program
 *                          written by Jungho Moon
 * ----------------------------------------------------------------------------
 */

#ifndef BSP_H
#define BSP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define OSC_FREQUENCY       16000000L   // 16 MHz
#define FIRMWARE_VERSION    0x0001      // firmware version

// PORTB
#define SD_CS       PB0         // output: SPI CS signal for SD card (active low)
#define SCLK        PB1         // output: SCLK (SPI)
#define MOSI        PB2         // output: MOSI (SPI)
#define MISO        PB3         // input:  MISO (SPI)
#define FLASH1_CS   PB4         // output: SPI CS for serial flash memory 1
#define FLASH2_CS   PB5         // output: SPI CS for serial flash memory 2
#define LCD_BL_EN   PB7         // output: LCD backlight enable (active low, OC2 for brightness control)

// PORTC
#define VCC3_EN     PC7         // output: 3 V regulator enable signal (active high)

// PORTD
#define MP3_xCS     PD5         // output: SPI xCS signal for MP3 codec (active low)
#define MP3_xDCS    PD6         // output: SPI xDCS signal for MP3 codec (active low)

// PORTE

// PORTF
#define SD_DET      PF6         // input:  SD card detection (H: no card, pull-up resistor required)
#define SD_WP       PF7         // input:  SD card write protection status (H: write protected, pull-up resistor required)

// MACRO functions
#define v30_enable()            PORTC |= _BV(VCC3_EN)
#define v30_disable()           PORTC &= ~_BV(VCC3_EN)
#define sd_card_check()         (!(PINF & _BV(SD_DET)))
#endif


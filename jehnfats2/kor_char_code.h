/*
 * ----------------------------------------------------------------------------
 *          Header file for Korean character code conversion functions
 *                              written by Jungho Moon
 * ----------------------------------------------------------------------------
 */

#ifndef KOR_CHAR_CODE_H
#define KOR_CHAR_CODE_H

#define KOR_CHAR_CODE

#ifdef  KOR_CODE_GLOBALS
#define KOR_CODE_EXT
#else
#define KOR_CODE extern
#endif

// global variables
extern prog_uint8_t table_initial[21];
extern prog_uint8_t table_vowel[30];
extern prog_uint8_t table_final[30];
extern prog_uint8_t bul_initial[2][22];
extern prog_uint8_t bul_final[22];

// function prototypes
uint16_t korean_ks2kssm_converter(uint16_t);
uint16_t korean_kssm2ks_converter(uint16_t);
uint16_t korean_unicode2kssm_converter(uint16_t);
uint16_t korean_unicode2ks_converter(uint16_t);
#endif

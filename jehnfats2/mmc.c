/*
 * ----------------------------------------------------------------------------
 * MMC/SD/SDHC card interface functions
 *
 * Author       : Jungho Moon
 * Target MCU   : ATMEL AVR ATmega64/128
 * ----------------------------------------------------------------------------
 */

#include "bsp.h"
#include "spi.h"
#include "mmc.h"

uint8_t sdcard_type;

/* ----------------------------------------------------------------------------
 * send an MMC command through the SPI interface and returns the response
 * arguments:
 *  - cmd: mmc command to send
 *  - arg: argument associated with the command
 * return value
 *  - response from the flash memory card
 * -------------------------------------------------------------------------- */
static uint8_t mmc_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t resp, retry=0;

    // send the command
    spi_read_write_byte(cmd|0x40);
    spi_read_write_byte(arg>>24);
    spi_read_write_byte(arg>>16);
    spi_read_write_byte(arg>>8);
    spi_read_write_byte(arg);
    
    switch(cmd) {
        case CMD_GO_IDLE_STATE:
            spi_read_write_byte(0x95);
            break;
        case CMD_SEND_IF_COND:
            spi_read_write_byte(0x87);
            break;
        default:
            spi_read_write_byte(0xff);
            break;
    }    

    // wait for response
    // time-out occurs after 8 tries
    while((resp = spi_read_write_byte(0xff)) == 0xff) {
        if(retry++ > 8)
            break;
    }

    return resp;
}

/* ----------------------------------------------------------------------------
 * initialize MMC/SD/SDHC flash memory
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
int8_t mmc_init(void)
{
    uint8_t resp, retry, spcr_backup;

    // configure chip select pin as output
    MMC_CS_DDR |= _BV(MMC_CS_PIN);
    MMC_CS_PORT |= _BV(MMC_CS_PIN);

    // apply more than 74 clocks with CS high before accessing the card with the SPI protocol
    for(resp=0; resp<10; resp++)
        spi_read_write_byte(0xff);

   // save SPI control register and lower the SPI clock frequency (fsck = fosc/64 with SPI2X of 1)
    spcr_backup = SPCR;
    SPCR = (spcr_backup & 0xfc) | _BV(SPR1) | _BV(SPR0);
    mmc_cs_assert();                        // assert SD CARD SPI enable signal

    // reset SD card
    retry = 0;
    do {
        resp = mmc_send_cmd(CMD_GO_IDLE_STATE, 0);
        if(retry++>20) {
            mmc_cs_deassert();
            SPCR = spcr_backup;
            return -1;                      // initialization error
        }
    } while(resp!=R1_IDLE_STATE);

    // check for SD card version
    sdcard_type = 0;
    resp = mmc_send_cmd(CMD_SEND_IF_COND, 0x100 | 0xaa);        // 0x100: voltage 2.7 v ~ 3.6 v, 0xaa: test pattern     
    if((resp & R1_ILLEGAL_CMD) == 0) {
        spi_read_write_byte(0xff);
        spi_read_write_byte(0xff);        
        spi_read_write_byte(0xff);        
        if(spi_read_write_byte(0xff) != 0xaa) {
            mmc_cs_deassert();
            SPCR = spcr_backup;            
            return -3;                      // wrong test pattern
        }
        sdcard_type |= SD_CARD_TYPE_SDV2;   // SD ver 2.0
    }        
    else {
        mmc_send_cmd(CMD_APP, 0);
        resp = mmc_send_cmd(CMD_SD_SEND_OP_COND, 0);
        if((resp & R1_ILLEGAL_CMD) == 0) {
            sdcard_type |= SD_CARD_TYPE_SDV1;
        }
        else {
            // mmc card
        }
    }
    
    // wait for card to get ready
    retry = 0;
    do {
        if(sdcard_type & (SD_CARD_TYPE_SDV2|SD_CARD_TYPE_SDV1)) {
            mmc_send_cmd(CMD_APP, 0);        
            resp = mmc_send_cmd(CMD_SD_SEND_OP_COND, (sdcard_type & SD_CARD_TYPE_SDV2)? 0x40000000L : 0);
        }    
        else
            resp = mmc_send_cmd(CMD_SEND_OP_COND, 0);

        if(retry++>20) {
            mmc_cs_deassert();
            SPCR = spcr_backup;
            return -4;                              // initialization error
        }
    } while(resp&R1_IDLE_STATE);

    if(sdcard_type & SD_CARD_TYPE_SDV2) {
        if(mmc_send_cmd(CMD_READ_OCR, 0)) {
            mmc_cs_deassert();
            SPCR = spcr_backup;
            return -5; 
        }
        if(spi_read_write_byte(0xff) & 0x40)
            sdcard_type |= SD_CARD_TYPE_SDHC;       // SDHC card
        
        spi_read_write_byte(0xff);
        spi_read_write_byte(0xff);
        spi_read_write_byte(0xff);
    }      

   // turn off CRC checking for simple communication
    mmc_send_cmd(CMD_CRC_ON_OFF, 0);

    // set block length to 512 bytes
    mmc_send_cmd(CMD_SET_BLOCKLEN, MMC_BLOCK_SIZE);

    mmc_cs_deassert();                      // deassert SD CARD SPI enable signal
    SPCR = spcr_backup;                     // resotre the content of SPCR (switch to highest SPI frequency)

   // release MISO
    spi_read_write_byte(0xff);
    
    return 0;
}

/* ----------------------------------------------------------------------------
 * read a block from MMC/SD/SDHC flash memory to buffer
 * arguments:
 *  - block: block number
 *  - buffer: pointer to the buffer
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
uint8_t mmc_read_block(uint32_t block, uint8_t *buffer)
{
    uint8_t resp;
    uint16_t i;

    mmc_cs_assert();                    // assert chip select
    
    if(sdcard_type & SD_CARD_TYPE_SDHC) {
        // sector addressing for SDHC card
        resp = mmc_send_cmd(CMD_READ_SINGLE_BLOCK, block);      
    }
    else {
        // byte addressing for SD card
        resp = mmc_send_cmd(CMD_READ_SINGLE_BLOCK, block<<9);    
    }    

    if(resp) {                          // check for valid response
        mmc_cs_deassert();
        return resp;
    }

    //wait for block start
    while(spi_read_write_byte(0xff) != MMC_STARTBLOCK_READ) ;

    // read data
    for(i=0; i<MMC_BLOCK_SIZE; i++)
        *buffer++ = spi_read_write_byte(0xff);

    // read 16-bit CRC
    spi_read_write_byte(0xff);
    spi_read_write_byte(0xff);
    
    mmc_cs_deassert();                  // deassert chip select

    // release MISO
    spi_read_write_byte(0xff);
    
    return 0;
}

/* ----------------------------------------------------------------------------
 * write a block from buffer to MMC/SD/SDHC flash memory card
 * arguments:
 *  - block: block number
 *  - buffer: pointer to the buffer
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
uint8_t mmc_write_block(uint32_t block, uint8_t *buffer)
{
    uint8_t resp;
    uint16_t i;

    mmc_cs_assert();                    // assert chip select

    if(sdcard_type & SD_CARD_TYPE_SDHC) {
        // sector addressing for SDHC card
        resp = mmc_send_cmd(CMD_WRITE_SINGLE_BLOCK, block);      
    }
    else {
        // byte addressing for SD card
        resp = mmc_send_cmd(CMD_WRITE_SINGLE_BLOCK, block<<9);    
    }    

    if(resp) {                          // check for valid response
        mmc_cs_deassert();
        return resp;
    }

    spi_read_write_byte(0xff);                   // dummy data
    spi_read_write_byte(MMC_STARTBLOCK_WRITE);   // data start token

    // data write
    for(i=0; i<MMC_BLOCK_SIZE; i++)
        spi_read_write_byte(*buffer++);

    spi_read_write_byte(0xff);              // discard 16-bt CRC value
    spi_read_write_byte(0xff);              // discard 16-bt CRC value

    // read data response token
    resp = spi_read_write_byte(0xff);
    if((resp&MMC_DR_MASK) != MMC_DR_ACCEPT) {
        mmc_cs_deassert();                  // deassert chip select
        return resp;
    }

    // wait until card not busy
    while(!spi_read_write_byte(0xff)) ;
    mmc_cs_deassert();                      // deassert chip select

    // release MISO
    spi_read_write_byte(0xff);
    
    return 0;
}

/* ----------------------------------------------------------------------------
 * read card's CID and CSD registers
 * arguments:
 *  - mmc_card_info: pointer to the buffer to store the card information
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
uint8_t mmc_get_card_info(MMC_CARD_INFO *mmc_card_info)
{
    uint8_t i, resp, buffer[16];
    uint8_t csd_read_bl_len, csd_c_size_mult;
    uint32_t csd_c_size;
    
    memset(mmc_card_info, 0, sizeof(MMC_CARD_INFO));
    
    mmc_cs_assert();                    // assert chip select
    
    // read card idenfitication data (CID) register
    if(resp = mmc_send_cmd(CMD_SEND_CID, 0)) {
        mmc_cs_deassert();
        return resp;
    }
    
    while(spi_read_write_byte(0xff) != 0xfe);
    
    for(i=0; i<16; i++) {
        buffer[i] = spi_read_write_byte(0xff);
    }
    
    spi_read_write_byte(0xff);              // discard 16-bt CRC value
    spi_read_write_byte(0xff);              // discard 16-bt CRC value
    
    mmc_card_info->manufacturer = buffer[0];
    memcpy((void *)(&(mmc_card_info->oem[0])), (const void *)(buffer+1), 2);
    memcpy((void *)(&(mmc_card_info->product[0])), (const void *)(buffer+3), 5);
    mmc_card_info->manufacturer = buffer[8];
    mmc_card_info->serial = (((uint32_t)buffer[9])<<24) | (((uint32_t)buffer[10])<<16) | (((uint32_t)buffer[11])<<8) | buffer[12];
    mmc_card_info->manufacture_year = (buffer[13]<<4) | (buffer[14]>>4);
    mmc_card_info->manufacture_month = buffer[14]&0x0f;
    
    // read card specific data (CSD) register
    csd_read_bl_len = csd_c_size_mult = csd_c_size = 0;
    if(resp = mmc_send_cmd(CMD_SEND_CSD, 0)) {
        mmc_cs_deassert();
        return resp;
    }
    
    while(spi_read_write_byte(0xff) != 0xfe);
    
    for(i=0; i<16; i++) {
        buffer[i] = spi_read_write_byte(0xff);
    }
    
    spi_read_write_byte(0xff);              // discard 16-bt CRC value
    spi_read_write_byte(0xff);              // discard 16-bt CRC value
    
    if(sdcard_type & SD_CARD_TYPE_SDHC) {        
        csd_c_size = (((uint32_t)(buffer[7]&0x3f))<<16) | ((buffer[8])<<8) | (buffer[9]);
        mmc_card_info->capacity = (uint64_t)(csd_c_size+1)<<19;
    }
    else {
        csd_read_bl_len = buffer[5] & 0x0f;
        csd_c_size = ((buffer[6] & 0x03)<<10) | ((buffer[7])<<2) | (buffer[8]>>6);
        csd_c_size_mult = ((buffer[9] & 0x03)<<1) | (buffer[10]>>7);
        mmc_card_info->capacity = (csd_c_size+1)<<(csd_c_size_mult + csd_read_bl_len + 2);
    }
 
    mmc_cs_deassert();                  // deassert chip select
    
    return 0;
}


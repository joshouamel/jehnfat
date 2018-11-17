/*
 * ----------------------------------------------------------------------------
 *          Header file for MMC/SD/SDHC card interface functions
 *                              written by Jungho Moon
 * ----------------------------------------------------------------------------
 */
 
#ifndef MMC_H
#define MMC_H

// SPI CS pin configuration
#define MMC_CS_PIN      PB0
#define MMC_CS_DDR      DDRB
#define MMC_CS_PORT     PORTB

// MMC commands
#define CMD_GO_IDLE_STATE           0x00        /* CMD0:  response R1 */
#define CMD_SEND_OP_COND            0x01        /* CMD1:  response R1 */
#define CMD_SEND_IF_COND            0x08        /* CMD8:  response R7 */
#define CMD_SEND_CSD                0x09        /* CMD9:  response R1 */
#define CMD_SEND_CID                0x0a        /* CMD10: response R1 */
#define CMD_STOP_TRANSMISSION       0x0c        /* CMD12: response R1 */
#define CMD_SEND_STATUS             0x0d        /* CMD13: response R2 */
#define CMD_SET_BLOCKLEN            0x10        /* CMD16: arg0[31:0]: block length, response R1 */
#define CMD_READ_SINGLE_BLOCK       0x11        /* CMD17: arg0[31:0]: data address, response R1 */
#define CMD_READ_MULTIPLE_BLOCK     0x12        /* CMD18: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_SINGLE_BLOCK      0x18        /* CMD24: arg0[31:0]: data address, response R1 */
#define CMD_WRITE_MULTIPLE_BLOCK    0x19        /* CMD25: arg0[31:0]: data address, response R1 */
#define CMD_PROGRAM_CSD             0x1b        /* CMD27: response R1 */
#define CMD_SET_WRITE_PROT          0x1c        /* CMD28: arg0[31:0]: data address, response R1b */
#define CMD_CLR_WRITE_PROT          0x1d        /* CMD29: arg0[31:0]: data address, response R1b */
#define CMD_SEND_WRITE_PROT         0x1e        /* CMD30: arg0[31:0]: write protect data address, response R1 */
#define CMD_TAG_SECTOR_START        0x20        /* CMD32: arg0[31:0]: data address, response R1 */
#define CMD_TAG_SECTOR_END          0x21        /* CMD33: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_SECTOR            0x22        /* CMD34: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_START   0x23        /* CMD35: arg0[31:0]: data address, response R1 */
#define CMD_TAG_ERASE_GROUP_END     0x24        /* CMD36: arg0[31:0]: data address, response R1 */
#define CMD_UNTAG_ERASE_GROUP       0x25        /* CMD37: arg0[31:0]: data address, response R1 */
#define CMD_ERASE                   0x26        /* CMD38: arg0[31:0]: stuff bits, response R1b */
#define CMD_SD_SEND_OP_COND         0x29        /* ACMD41: arg0[31:0]: OCR contents, response R1 */
#define CMD_LOCK_UNLOCK             0x2a        /* CMD42: arg0[31:0]: stuff bits, response R1b */
#define CMD_APP                     0x37        /* CMD55: arg0[31:0]: stuff bits, response R1 */
#define CMD_READ_OCR                0x3a        /* CMD58: arg0[31:0]: stuff bits, response R3 */
#define CMD_CRC_ON_OFF              0x3b        /* CMD59: arg0[31:1]: stuff bits, arg0[0:0]: crc option, response R1 */

// R1 Response definitions
#define R1_BUSY                     0x80        // R1 response: bit indicates card is busy
#define R1_PARAMETER                0x40
#define R1_ADDRESS                  0x20
#define R1_ERASE_SEQ                0x10
#define R1_COM_CRC                  0x08
#define R1_ILLEGAL_CMD              0x04
#define R1_ERASE_RESET              0x02
#define R1_IDLE_STATE               0x01

// Data Start tokens
#define MMC_STARTBLOCK_READ         0xFE        // when received from card, indicates that a block of data will follow
#define MMC_STARTBLOCK_WRITE        0xFE        // when sent to card, indicates that a block of data will follow
#define MMC_STARTBLOCK_MWRITE       0xFC

// Data Stop tokens
#define MMC_STOPTRAN_WRITE          0xFD

// Data Error Token values
#define MMC_DE_MASK                 0x1F
#define MMC_DE_ERROR                0x01
#define MMC_DE_CC_ERROR             0x02
#define MMC_DE_ECC_FAIL             0x04
#define MMC_DE_OUT_OF_RANGE         0x04
#define MMC_DE_CARD_LOCKED          0x04

// Data Response Token values
#define MMC_DR_MASK                 0x1F
#define MMC_DR_ACCEPT               0x05
#define MMC_DR_REJECT_CRC           0x0B
#define MMC_DR_REJECT_WRITE_ERROR   0x0D

// MMC block size in bytes
#define MMC_BLOCK_SIZE              512

/* status bits for card types */
#define SD_CARD_TYPE_SDV1           0x01
#define SD_CARD_TYPE_SDV2           0x02
#define SD_CARD_TYPE_SDHC           0x04

typedef struct 
{
	uint8_t manufacturer;       // manufacturer ID globally assigned by the SD card organization
	uint8_t oem[3];             // string describing the card's OEM or content, globally assigned by the SD card organization
	uint8_t product[6];         // product name
	uint8_t revision;           // card's revision, coded in packed BCD
	uint32_t serial;            // A serial number assigned by the manufacturer
	uint8_t manufacture_year;   // The year of manufacturing. A value of zero means year 2000
	uint8_t manufacture_month;  // The month of manufacturing
	uint64_t capacity;          // capacity in bytes for SD cards and in 512 kb for SDHC cards
} MMC_CARD_INFO;

// macro functions
#define mmc_cs_assert()         MMC_CS_PORT &= ~_BV(MMC_CS_PIN)
#define mmc_cs_deassert()       MMC_CS_PORT |= _BV(MMC_CS_PIN)

// function prototypes
int8_t mmc_init(void);
uint8_t mmc_read_block(uint32_t, uint8_t *);
uint8_t mmc_write_block(uint32_t, uint8_t *);
uint8_t mmc_get_card_info(MMC_CARD_INFO *);
#endif

/*
 * ----------------------------------------------------------------------------
 * FAT32 demo with SD/SDHC card
 *                         
 * Author       : Jungho Moon
 * Target MCU   : ATMEL AVR ATmega128/64
 *
 * 이 예제는 FAT32 파일 시스템을 이용하여 SD/SDHC 메모리 카드의 내용을 읽는 
 * 방법을 설명하기 위한 것이다. 
 *
 * MS-DOS 명령어 dir, cd, type, vol외에 dirs라는 명령어를 사용할 수 있다.
 * dirs는 디렉터리의 내용을 조금 더 상세하게 보여주는 명령이다.
 *
 * 파일이나 디렉터리 이름에 공백이 있는 경우에는 이름을 "  "로 묶는다.
 * ----------------------------------------------------------------------------
 */

#define SYS_GLOBALS

#include "bsp.h"
#include "uart.h"
#include "spi.h"
#include "mmc.h"
#include "fat32.h"
#include "fat32_demo.h"

/* ----------------------------------------------------------------------------
 * generate a delay in seconds (maximum value: 25.5)
 * -------------------------------------------------------------------------- */
static void _delay_s(float seconds)
{
    uint16_t i;
    
    for(i=0; i<(uint16_t)(100*seconds+0.5); i++)
        _delay_ms(10);
}

/* ----------------------------------------------------------------------------
 * handler for timer0 compare match interrupt
 * -------------------------------------------------------------------------- */
ISR(TIMER0_COMP_vect)
{
}

/* ----------------------------------------------------------------------------
 * handler for external interrupt INT7
 * -------------------------------------------------------------------------- */
ISR(INT7_vect)
{
}

/* ----------------------------------------------------------------------------
 * initialize I/O ports
 * -------------------------------------------------------------------------- */
void ioport_init(void)
{
    /* SD 카드만 test하기를 원할 때에도 SPI 버스에 여러 디바이스가 공통으로
     * 연결되어 있다면 SD 카드를 제외한 다른 모든 SPI 장치의 CS는 'H'로 설정해
     * disable시켜야만 SD 카드와의 통신에 간섭을 일으키지 않는다. */
        
    // PORT B
    DDRB = _BV(SD_CS) | _BV(FLASH1_CS) | _BV(FLASH2_CS) | _BV(LCD_BL_EN);
    PORTB = _BV(SD_CS) | _BV(FLASH1_CS) | _BV(FLASH2_CS) | _BV(LCD_BL_EN);
    
    // PORT C
    DDRC = _BV(VCC3_EN);
    
    // PORT D
    DDRD = _BV(MP3_xCS) | _BV(MP3_xDCS);
    PORTD = _BV(MP3_xCS) | _BV(MP3_xDCS);

    // PORT F
    PORTF = _BV(SD_DET) | _BV(SD_WP);           // enable pullup resistors
}

/* ----------------------------------------------------------------------------
 * initialize timers
 * -------------------------------------------------------------------------- */
void timer_init(void)
{
}

/* ----------------------------------------------------------------------------
 * initialize ADC
 * -------------------------------------------------------------------------- */
void adc_init(void)
{
}

/* ----------------------------------------------------------------------------
 * initialize interrupt-related SFRs
 * -------------------------------------------------------------------------- */
void interrupt_init(void)
{
}

/* ----------------------------------------------------------------------------
 * initialize ATmega128 on-chip peripherals
 * -------------------------------------------------------------------------- */
void atmega128_peripherals_init(void)
{
    // initialize on-chip peripherals
    ioport_init();
    timer_init();
    spi_master_init();
    adc_init();
    uart_init();
    interrupt_init();    
}

/* ----------------------------------------------------------------------------
 * initialize off-chip peripherals
 * -------------------------------------------------------------------------- */
void off_chip_peripherals_init(void)
{
    v30_enable();                           // enable 3.0V regulator
    _delay_ms(1);                           // wait until 3.0V power is stabilized
}
    
/* ----------------------------------------------------------------------------
 * entry point to the program
 * -------------------------------------------------------------------------- */
int16_t main(void)
{
    int8_t ch, *ptr;
    int8_t command[20];
    int8_t filename[LONG_FILE_NAME_BUF_LEN];
    int8_t typed_command[LONG_FILE_NAME_BUF_LEN + 20];
    uint16_t i, num_dirs, num_files;
    uint32_t de_physical_addr;
    DIRECTORY_ENTRY dir_entry;
    MFILE *fp;

    fdevopen(uart_putch, uart_getch);   // printf are directed to UART
    atmega128_peripherals_init();
    off_chip_peripherals_init();
    
    if(!sd_card_check()) {
        printf(">>\n\r SD/SDHC CARD NOT FOUND. INSERT A CARD.\r\n");
        while(!sd_card_check()) ;
        _delay_s(0.2);
    } 
    
    if(mmc_init()) {
        printf(">>\n\r SD/SDHC MEMORY CARD INITIALIZATION FAILURE! RESET THE BOARD.\n\r"); 
        while(1) ;
    }
    
    if(fat32_init()) {
        printf(">>\n\r FAT32 FILE SYSTEM NOT FOUND ON THE MEMORY CARD.\n\r"); 
        while(1) ;
    }
 
    while(1) {
        uart_puts("\n\r>> ");       
        
        i = 0;
        do {       
            ch = uart_getch();
            uart_putch(ch);
            typed_command[i++] = ch;
        } while(ch != '\r');        

        typed_command[i-1] = '\0';                               
        i = sscanf(typed_command, "%s %s", command, filename);
        
        if(i==0)
            continue;
        else if(i==1) {
            if(!strcasecmp(command, "dir")) {           // if command is "dir" 
                num_files = num_dirs = 0;
                printf("\n\r");
                
                de_physical_addr = fat32_get_first_file_info(fat32_get_current_dir_cluster(), filename, &dir_entry);
                while(de_physical_addr) {
                    // 현재 디렉터리의 내용을 보여줌
                    if(dir_entry.deAttributes & FILE_ATTR_DIRECTORY) {
                        num_dirs++;
                        printf(" <DIR> ");
                    }
                    else {
                        num_files++;
                        printf("       ");
                    }
                    printf(" %s\n\r", filename);
                    de_physical_addr = fat32_get_next_file_info(de_physical_addr, filename, &dir_entry);
                }
                printf("\n\r      %3d FILES \n\r      %3d DIRECTORIES\n\r", num_files, num_dirs);
                
                continue;
            }
            else if(!strcasecmp(command, "dirs")) {     // if command is "dirs"            
                fat32_show_directory(fat32_get_current_dir_cluster());
            }
            else if(!strcasecmp(command, "vol")) {      // if command is "vol"            
                fat32_get_volume_label(filename);
                printf("\n\r Volume label: %s\n\r\n", filename);
            }
        }
        else if(i==2){
            // 파일 이름이 "로 시작하는 경우 처리
            if(filename[0] == '\"'){
                ptr = strstr(typed_command, filename);
                strcpy(filename, ptr+1);
                if((strlen(filename) > 1) && (filename[strlen(filename)-1] == '\"'))
                    filename[strlen(filename)-1] = '\0';
            }
            
            if(!strcasecmp(command, "cd")) {            // if command is "cd"            
                if(!fat32_chdir(filename))
                    printf("\n\r DIRECTORY CHANGED\n\r");
                else
                    printf("\n\r PATH NOT FOUND\n\r");
            }
            else if(!strcasecmp(command, "type")) {     // if command is "type"            
                if(fp = fat32_fopen(filename)) {
                    printf("\n\r\n FILE SIZE: %ld BYTES\n\r\n", fp->size);

                    // text file 출력
                    while((ch = fat32_fgetc(fp)) != EOF)          
                        uart_putch(ch);

                    printf("\n\r");
                    fat32_fclose(fp);
                }
                else
                    printf("\n\r FILE NOT FOUND\n\r");

/*
                // 바이너리 파일을 읽거나 텍스트 파일을 읽는 속도를 높이려면 fat32_fread()를 사용
                if(fp = fat32_fopen(filename)) {
                    printf("\n\r\n FILE SIZE: %ld BYTES\n\r\n", fp->size);

                    do {
                        length = fat32_fread(fp);
                        for(i=0; i<length; i++)
                            uart_putch(fp->buffer[i]);
                    } while(length ==  BYTES_PER_SECTOR);       
                    
                    printf("\n\r");
                    fat32_fclose(fp);
                }
                else
                    printf("\n\r FILE NOT FOUND\n\r");
*/                    
            }
        }
    }
    
    return 0;
}


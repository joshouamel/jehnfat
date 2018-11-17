/*
 * ----------------------------------------------------------------------------
 * FAT32 file system library
 *  - long filenames and Korean characters are supported
 *  - multiple partitions and write functions are not supported yet
 *
 * Author       : Jungho Moon
 * Target MCU   : ATMEL AVR ATmega64/128
 * ----------------------------------------------------------------------------
 */

#define FAT32_GLOBALS
#define SHOW_DIRECTORY

#ifdef  FAT32_GLOBALS
#define FAT32_EXT
#else
#define FAT32_EXT extern
#endif

#include "bsp.h"
#include "mmc.h"
#include "fat32.h"
#include "kor_char_code.h"

FAT32_INFO fat32_info;
uint32_t current_dir_cluster=2;
uint8_t mmc_buffer[BYTES_PER_SECTOR];

prog_uint8_t fat32_long_file_name_index_order[] =
{
    1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30
};

/* ----------------------------------------------------------------------------
 * returns the number of sectors per cluster
 * -------------------------------------------------------------------------- */
static uint8_t fat32_get_sectors_per_cluster(void)
{
    return fat32_info.sectors_per_cluster;
}

/* ----------------------------------------------------------------------------
 * returns the root cluster number
 * -------------------------------------------------------------------------- */
static uint32_t fat32_get_root_cluster(void)
{
    return fat32_info.root_cluster;
}

/* ----------------------------------------------------------------------------
 * converts cluster number to sector number
 * arguments
 *  - cluster: cluster number
 * return value
 *  - the first sector number corresponding to the cluster number
 * -------------------------------------------------------------------------- */
static uint32_t fat32_cluster2sector(uint32_t cluster)
{
    if(cluster == 0 || cluster == 1)
        cluster = 2;

    return ((cluster-2)*fat32_info.sectors_per_cluster + fat32_info.first_data_sector);
}

/* ----------------------------------------------------------------------------
 * converts sector number to cluster number
 * arguments
 *  - sector: sector number
 * return value
 *  - cluster number corresponding to the sector
 * -------------------------------------------------------------------------- */
static uint32_t fat32_sector2cluster(uint32_t sector)
{
    return ((sector- fat32_info.first_data_sector)/fat32_info.sectors_per_cluster + 2);
}

/* ----------------------------------------------------------------------------
 * find next cluster in the FAT chain
 * arguments
 *  - cluster: current cluster number
 * return value
 *  - next cluster number if exsists
 * -------------------------------------------------------------------------- */
static uint32_t fat32_find_next_cluster(uint32_t cluster)
{
    uint16_t offset;
    uint32_t fat_offset, sector, next_cluster;

    if(cluster==0)
        cluster = 2;

    fat_offset = cluster<<2;                // 4 bytes for every cluster entry
    sector = fat32_info.first_fat_sector + (fat_offset/BYTES_PER_SECTOR);
    offset = fat_offset % BYTES_PER_SECTOR;

    mmc_read_block(sector, mmc_buffer);
    next_cluster = *((uint32_t *)(mmc_buffer+offset)) & FAT32_MASK;

    if(next_cluster == (CLUST_EOFE&FAT32_MASK))
        next_cluster = CLUST_EOFE;

    return next_cluster;
}

/* ----------------------------------------------------------------------------
 * return current directory cluster
 * -------------------------------------------------------------------------- */
uint32_t fat32_get_current_dir_cluster(void)
{
    return current_dir_cluster;
}

/* ----------------------------------------------------------------------------
 * set current directory cluster
 * -------------------------------------------------------------------------- */
static void fat32_set_current_dir_cluster(uint32_t dir_cluster)
{
    current_dir_cluster = dir_cluster;
}

/* ----------------------------------------------------------------------------
 * intialize FAT32 data structure
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
int8_t fat32_init(void)
{
    uint32_t start_lba;
    PARTION_ENTRY *partion_entry;

    mmc_read_block(0, mmc_buffer);                              // read MBR
    partion_entry = (PARTION_ENTRY *)(mmc_buffer + 0x1be);      // offset 0x1be points to the 1st partition entry;
    start_lba = partion_entry->peStartLBA;                      // get the start LBA of the first PBR

    if(partion_entry->pePartType != PART_TYPE_FAT32)
        return -1;

    mmc_read_block(start_lba, mmc_buffer);                         // read the boot sector (PBR)
    fat32_info.sectors_per_cluster = ((BOOT_SECTOR *)mmc_buffer)->bsSecPerClust;
    fat32_info.root_cluster = ((BOOT_SECTOR *)mmc_buffer)->bsRootClust;
    fat32_info.sectors_per_fat = ((BOOT_SECTOR *)mmc_buffer)->bsBigFATsecs;
    fat32_info.first_fat_sector = start_lba + ((BOOT_SECTOR *)mmc_buffer)->bsResSectors;
    fat32_info.first_data_sector = fat32_info.first_fat_sector + (((BOOT_SECTOR *)mmc_buffer)->bsBigFATsecs)*(((BOOT_SECTOR *)mmc_buffer)->bsFATs);

    return 0;
}

/* ----------------------------------------------------------------------------
 * get directory entry for a file with the given name
 * 지정한 클러스터에서 인자로 주어진 파일의 directory entry를 가져온다.
 * 파일 이름에는 path를 지원하지 않는다.
 * arguments:
 *  - cluster: cluster number
 *  - filename: pointer to the string containing filename (path not allowed)
 *  - de: address of the directory entry structure to be filled with the file info
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
int8_t fat32_get_dir_entry(uint32_t cluster, int8_t *filename, DIRECTORY_ENTRY *de)
{
    uint8_t i, j, k, l;
    int8_t long_name_flag=0, long_filename[LONG_FILE_NAME_BUF_LEN];
    uint16_t seq_num, unicode, kssm;
    uint32_t sector;
    DIRECTORY_ENTRY *dir_entry;

    do {
        sector = fat32_cluster2sector(cluster);
        for(l=0; l<fat32_get_sectors_per_cluster(); l++) {
            mmc_read_block(sector, mmc_buffer);
            dir_entry = (DIRECTORY_ENTRY *)mmc_buffer;
            for(i=0; i<(BYTES_PER_SECTOR/32); i++) {
                if(dir_entry->deName[0] == FILE_HEADER_EMPTY) {
                    // there is no more directory entries
                    return -1;      
                }
    
                if(dir_entry->deName[0] != FILE_HEADER_DELETED) {
                    if(dir_entry->deAttributes != FILE_ATTR_LONG_FILENAME) {
                        if(!(dir_entry->deAttributes & FILE_ATTR_VOLUME)) {
                            if(long_name_flag == 1) {
                                // if this entry is the last one preceded by long file name entries
                                long_name_flag = 0;
                                for(j=k=0; j<LONG_FILE_NAME_BUF_LEN/2; j++) {
                                    unicode = *((uint16_t *)long_filename+j);
                                    if(unicode == 0) {
                                        long_filename[k++] = 0;
                                        break;
                                    }
                                    else if(unicode <= 0x7f)        // English
                                        long_filename[k++] = (uint8_t)unicode;
                                    else {                          // Korean
                                        #ifdef KOR_CHAR_CODE
                                        kssm = korean_unicode2ks_converter(unicode);
                                        long_filename[k++] = kssm>>8;
                                        long_filename[k++] = kssm&0xff;
                                        #endif
                                    }
                                }
                            }
                            else {
                            // if this is a simple short file name entry
                                memset(long_filename, 0, 13);
                                for(j=0; j<8; j++) {
                                    if(dir_entry->deName[j] != ' ')
                                        long_filename[j] = tolower(dir_entry->deName[j]);
                                    else
                                        break;
                                }

                                if(dir_entry->deExtension[0] != ' ') {
                                    long_filename[j++] = '.';
                                    for(k=0; k<3; k++) {
                                        if(dir_entry->deExtension[k] != ' ')
                                            long_filename[j++] = tolower(dir_entry->deExtension[k]);
                                        else
                                            break;
                                    }
                                }
                            }
                            if(!strcasecmp(long_filename, filename)) {
                                memcpy(de, dir_entry, sizeof(DIRECTORY_ENTRY));
                                return 0;
                            }
                        }
                    }
                    else {
                        if((dir_entry->deName[0] & 0x80) == 0) { 
                            seq_num = 26*((dir_entry->deName[0] & 0x3f) - 1);
                            if(long_name_flag == 0) {
                                long_name_flag = 1;
                                memset(long_filename, 0, LONG_FILE_NAME_BUF_LEN);
                            }

                            for(j=0; j<13; j++) {
                                long_filename[seq_num+2*j] = *(((uint8_t *)dir_entry) +
                                                                pgm_read_byte(fat32_long_file_name_index_order+j));
                                long_filename[seq_num+2*j+1] = *(((uint8_t *)dir_entry) + 1 +
                                                                pgm_read_byte(fat32_long_file_name_index_order+j));
                                if((long_filename[seq_num+2*j] == 0) && (long_filename[seq_num+2*j+1] == 0))
                                    break;
                            }
                        }
                    }
                }
                dir_entry++;
            }
            sector++;
        }
        cluster = fat32_find_next_cluster(cluster);
    } while(cluster != CLUST_EOFE) ;
    
    return -1;
}

/* ----------------------------------------------------------------------------
 * get information on the first file in a cluster
 * 지정한 클러스터에 있는 첫번째 파일에 대한 정보 (파일 이름과 해당 디렉토리 엔트리)를 
 * 가져온다.
 * arguments:
 *  - cluster: cluster number
 *  - filename: pointer to the buffer where the file name is to be stored
 *  - de: pointer to the directory entry to be filled with the found information
 * return value
 *  - physical address of the dirctory entry in the memory card (0 indicates error)
 *    = sector number (27 bits) + offset (5 bits)
 *    offset is the order of the directory entry within the sector
 * -------------------------------------------------------------------------- */
uint32_t fat32_get_first_file_info(uint32_t cluster, int8_t *filename, DIRECTORY_ENTRY *de)
{
    uint8_t i, j, k, l;
    int8_t long_name_flag, long_filename[LONG_FILE_NAME_BUF_LEN];
    uint16_t seq_num, unicode, kssm;
    uint32_t sector;
    DIRECTORY_ENTRY *dir_entry;

    long_name_flag = 0;
    while(1) {
        sector = fat32_cluster2sector(cluster);
        for(l=0; l<fat32_get_sectors_per_cluster(); l++) {
            mmc_read_block(sector, mmc_buffer);
            dir_entry = (DIRECTORY_ENTRY *)mmc_buffer;

            for(i=0; i<(BYTES_PER_SECTOR/32); i++) {
                if(dir_entry->deName[0] == FILE_HEADER_EMPTY) {
                    // there is no more directory entries
                    return 0;
                }
                
                if(dir_entry->deName[0] != FILE_HEADER_DELETED) {
                    if(dir_entry->deAttributes != FILE_ATTR_LONG_FILENAME) {
                        if(!(dir_entry->deAttributes & FILE_ATTR_VOLUME)) {
                            if(long_name_flag == 1) {
                            // if this entry is the last one preceded by long file name entries
                                long_name_flag = 0;
                                for(j=k=0; j<LONG_FILE_NAME_BUF_LEN/2; j++) {
                                    unicode = *((uint16_t *)long_filename+j);
                                    if(unicode == 0) {
                                        long_filename[k++] = 0;
                                        break;
                                    }
                                    else if(unicode <= 0x7f)        // English
                                        long_filename[k++] = (uint8_t)unicode;
                                    else {                          // Korean
                                        #ifdef KOR_CHAR_CODE
                                        kssm = korean_unicode2ks_converter(unicode);
                                        long_filename[k++] = kssm>>8;
                                        long_filename[k++] = kssm&0xff;
                                        #endif
                                    }
                                }
                                strcpy(filename, long_filename);
                                memcpy(de, dir_entry, sizeof(DIRECTORY_ENTRY));
                                return ((sector<<5) + i);
                            }
                            else {
                            // if this is a simple short file name entry
                                memset(long_filename, 0, 13);
                                for(j=0; j<8; j++) {
                                    if(dir_entry->deName[j] != ' ')
                                        long_filename[j] = tolower(dir_entry->deName[j]);
                                    else
                                        break;
                                }

                                if(dir_entry->deExtension[0] != ' ') {
                                    long_filename[j++] = '.';
                                    for(k=0; k<3; k++) {
                                        if(dir_entry->deExtension[k] != ' ')
                                            long_filename[j++] = tolower(dir_entry->deExtension[k]);
                                        else
                                            break;
                                    }
                                }
                                strcpy(filename, long_filename);
                                memcpy(de, dir_entry, sizeof(DIRECTORY_ENTRY));
                                return ((sector<<5) + i);
                            }
                        }
                    }
                    else {
                        if((dir_entry->deName[0] & 0x80) == 0) { 
                            seq_num = 26*((dir_entry->deName[0] & 0x3f) - 1);
                            if(long_name_flag == 0) {
                                long_name_flag = 1;
                                memset(long_filename, 0, LONG_FILE_NAME_BUF_LEN);
                            }

                            for(j=0; j<13; j++) {
                                long_filename[seq_num+2*j] = *(((uint8_t *)dir_entry) +
                                                                pgm_read_byte(fat32_long_file_name_index_order+j));
                                long_filename[seq_num+2*j+1] = *(((uint8_t *)dir_entry) + 1 +
                                                                pgm_read_byte(fat32_long_file_name_index_order+j));
                                if((long_filename[seq_num+2*j] == 0) && (long_filename[seq_num+2*j+1] == 0))
                                    break;
                            }
                        }
                    }
                }
                dir_entry++;
            }
            sector++;
        }
        cluster = fat32_find_next_cluster(cluster);
    }
}

/* ----------------------------------------------------------------------------
 * get information on a next file in a cluster
 * 인자로 주어진 주소의 디렉토리 엔트리 바로 다음 엔트리의 정보를 가져온다.
 * arguments:
 *  - de_physical_addr: physical address of the previous dirctory entry in the memory card
 *  - filename: pointer to the buffer where the file name is to be stored
 *  - de: pointer to the directory entry to be filled with the found information
 * return value
 *  - physical address of the dirctory entry in the memory card (0 indicates error)
 *    = sector number (27 bits) + offset (5 bits)
 *    offset is the order of the directory entry within the sector
 * -------------------------------------------------------------------------- */
uint32_t fat32_get_next_file_info(uint32_t de_physical_addr, int8_t *filename, DIRECTORY_ENTRY *de)
{
    uint8_t i, j, k, offset;
    int8_t long_name_flag, long_filename[LONG_FILE_NAME_BUF_LEN];
    uint16_t seq_num, unicode, kssm;
    uint32_t sector, cluster;
    DIRECTORY_ENTRY *dir_entry;

    long_name_flag = 0;
    sector = de_physical_addr>>5;
    offset = (de_physical_addr&0x1f)+1;
    cluster = fat32_sector2cluster(sector);
    
    if(offset == (BYTES_PER_SECTOR/32)) {
        offset = 0;
        if(fat32_sector2cluster(sector) == fat32_sector2cluster(sector+1))
            sector++;
        else {
            cluster = fat32_find_next_cluster(cluster);
            sector = fat32_cluster2sector(cluster);
        }
    }

    while(1) {
        mmc_read_block(sector, mmc_buffer);
        dir_entry = (DIRECTORY_ENTRY *)mmc_buffer+offset;

        for(i=offset; i<(BYTES_PER_SECTOR/32); i++) {
            if(dir_entry->deName[0] == FILE_HEADER_EMPTY) {
                // there is no more directory entries
                return 0;
            }
            
            if(dir_entry->deName[0] != FILE_HEADER_DELETED) {
                if(dir_entry->deAttributes != FILE_ATTR_LONG_FILENAME) {
                    if(!(dir_entry->deAttributes & FILE_ATTR_VOLUME)) {
                        if(long_name_flag == 1) {
                        // if this entry is the last one preceded by long file name entries
                            long_name_flag = 0;
                            for(j=k=0; j<LONG_FILE_NAME_BUF_LEN/2; j++) {
                                unicode = *((uint16_t *)long_filename+j);
                                if(unicode == 0) {
                                    long_filename[k++] = 0;
                                    break;
                                }
                                else if(unicode <= 0x7f)        // English
                                    long_filename[k++] = (uint8_t)unicode;
                                else {                          // Korean
                                    #ifdef KOR_CHAR_CODE
                                    kssm = korean_unicode2ks_converter(unicode);
                                    long_filename[k++] = kssm>>8;
                                    long_filename[k++] = kssm&0xff;
                                    #endif
                                }
                            }
                            strcpy(filename, long_filename);
                            memcpy(de, dir_entry, sizeof(DIRECTORY_ENTRY));
                            return ((sector<<5) + i);
                        }
                        else {
                        // if this is a simple short file name entry
                            memset(long_filename, 0, 13);
                            for(j=0; j<8; j++) {
                                if(dir_entry->deName[j] != ' ')
                                    long_filename[j] = tolower(dir_entry->deName[j]);
                                else
                                    break;
                            }

                            if(dir_entry->deExtension[0] != ' ') {
                                long_filename[j++] = '.';
                                for(k=0; k<3; k++) {
                                    if(dir_entry->deExtension[k] != ' ')
                                        long_filename[j++] = tolower(dir_entry->deExtension[k]);
                                    else
                                        break;
                                }
                            }
                            strcpy(filename, long_filename);
                            memcpy(de, dir_entry, sizeof(DIRECTORY_ENTRY));
                            return ((sector<<5) + i);
                        }
                    }
                }
                else {
                    if((dir_entry->deName[0] & 0x80) == 0) { 
                        seq_num = 26*((dir_entry->deName[0] & 0x3f) - 1);
                        if(long_name_flag == 0) {
                            long_name_flag = 1;
                            memset(long_filename, 0, LONG_FILE_NAME_BUF_LEN);
                        }

                        for(j=0; j<13; j++) {
                            long_filename[seq_num+2*j] = *(((uint8_t *)dir_entry) +
                                                            pgm_read_byte(fat32_long_file_name_index_order+j));
                            long_filename[seq_num+2*j+1] = *(((uint8_t *)dir_entry) + 1 +
                                                            pgm_read_byte(fat32_long_file_name_index_order+j));
                            if((long_filename[seq_num+2*j] == 0) && (long_filename[seq_num+2*j+1] == 0))
                                break;                                                        
                        }
                    }
                }
            }
            dir_entry++;
        }

        offset = 0;
        if(fat32_sector2cluster(sector) == fat32_sector2cluster(sector+1))
            sector++;
        else {
            cluster = fat32_find_next_cluster(cluster);
            sector = fat32_cluster2sector(cluster);
        }
    }
}

/* ----------------------------------------------------------------------------
 * find cluster of a directory
 * arguments:
 *  - dir_name: pointer to the sting containing directory name 
 *              examples    fat32_chdir("\\data");
 *                          fat32_chdir("..\\menu\\items);
 *                          fat32_chdir("menu\\items);
 *                          fat32_chdir("data_dir");
 * return value
 *  - cluster number of the directory (0 indicates that directory does not exist)
 * -------------------------------------------------------------------------- */
static uint32_t fat32_find_dir_cluster(int8_t *dir_name)
{
    DIRECTORY_ENTRY de;
    int8_t *ptr, partial_name[LONG_FILE_NAME_BUF_LEN];
    uint32_t dir_cluster;

    if(dir_name[0] == '\\') {
        dir_cluster = fat32_get_root_cluster();
        if(*(++dir_name) == '\0') {
            return dir_cluster;
        }
    }
    else
        dir_cluster = fat32_get_current_dir_cluster();

    while((ptr = strchr(dir_name, '\\'))) {
        memcpy(partial_name, dir_name, ptr-dir_name);
        partial_name[ptr-dir_name] ='\0';
        if(fat32_get_dir_entry(dir_cluster, partial_name, &de))
            return 0;

        if(de.deAttributes & FILE_ATTR_DIRECTORY) {
            dir_cluster = ((uint32_t)de.deHighClust<<16) + de.deStartCluster;
            if(dir_cluster == 0)
                dir_cluster = 2;
        }
        else
            return 0;
        dir_name = ptr+1;
    }

    if(*dir_name != 0) {
        if(fat32_get_dir_entry(dir_cluster, dir_name, &de)) {
            return 0;
        }

        if(de.deAttributes & FILE_ATTR_DIRECTORY) {
            dir_cluster = ((uint32_t)de.deHighClust<<16) + de.deStartCluster;
            if(dir_cluster == 0)
                dir_cluster = 2;
            return dir_cluster;
        }
        else
            return 0;
    }
    else
        return 0;
}

/* ----------------------------------------------------------------------------
 * change directory
 * arguments:
 *  - dir_name: pointer to the sting containing directory name
 *              examples    fat32_chdir("\\data");
 *                          fat32_chdir("..\menu\\items);
 *                          fat32_chdir("menu\\items);
 *                          fat32_chdir("data_dir");
 * return value
 *  - error code (0 indicates no error)
 * -------------------------------------------------------------------------- */
int8_t fat32_chdir(int8_t *dir_name)
{
    uint32_t dir_cluster;

    dir_cluster = fat32_find_dir_cluster(dir_name);
    if(dir_cluster) {
        fat32_set_current_dir_cluster(dir_cluster);
        return 0;
    }
    else
        return -1;
}

/* ----------------------------------------------------------------------------
 * open a file
 * arguments:
 *  - filename: pointer to the sting containing filename name
 *              examples    fat32_fopen("\\data\filter.c");
 *                          fat32_fopen("..\menu\\items.dat);
 *                          fat32_fopen("data.xls");
 * return value
 *  - file pointer (0 indicates error)
 * -------------------------------------------------------------------------- */
MFILE *fat32_fopen(int8_t *filename)
{
    MFILE *fp;
    DIRECTORY_ENTRY de;
    int8_t *ptr, partial_name[LONG_FILE_NAME_BUF_LEN];
    uint32_t dir_cluster;

    if(filename[0] == '\\') {
        dir_cluster = fat32_get_root_cluster();
        if(*(++filename) == '\0') {
            return (MFILE *)0;
        }
    }
    else
        dir_cluster = fat32_get_current_dir_cluster();

    while((ptr = strchr(filename, '\\'))) {
        memcpy(partial_name, filename, ptr-filename);
        partial_name[ptr-filename] ='\0';

        if(fat32_get_dir_entry(dir_cluster, partial_name, &de))
            return (MFILE *)0;

        if(de.deAttributes & FILE_ATTR_DIRECTORY) {
            dir_cluster = ((uint32_t)de.deHighClust<<16) + de.deStartCluster;
            if(dir_cluster == 0)
                dir_cluster = 2;
        }
        else
            return (MFILE *)0;
        filename = ptr+1;
    }

    if(*filename != 0) {
        if(fat32_get_dir_entry(dir_cluster, filename, &de))
            return (MFILE *)0;

        if(!(de.deAttributes & FILE_ATTR_DIRECTORY)) {
            fp = (MFILE *)malloc(sizeof(MFILE));
            fp->size = de.deFileSize;
            fp->start_cluster = ((uint32_t)de.deHighClust<<16) + de.deStartCluster;
            fp->current_sector = fat32_cluster2sector(((uint32_t)de.deHighClust<<16) + de.deStartCluster);
            fp->pos_indicator = 0;
            return fp;
        }
        else
            return (MFILE *)0;
    }
    else
        return (MFILE *)0;
}

/* ----------------------------------------------------------------------------
 * close a file
 * arguments:
 *  - fp: file pointer
 *
 * return value
 *  - 0 (currently only 0)
 * -------------------------------------------------------------------------- */
uint8_t fat32_fclose(MFILE *fp)
{
    free(fp);

    return 0;
}

/* ----------------------------------------------------------------------------
 * reposition the file position indicator to the beginning of a file
 * -------------------------------------------------------------------------- */
void fat32_frewind(MFILE *fp)
{
    fp->pos_indicator = 0;
    fp->current_sector = fat32_cluster2sector(fp->start_cluster);
}

/* ----------------------------------------------------------------------------
 * rereposition the file position indicator to an arbitrary positon of a file
 * arguments:
 *  - fp: file pointer
 *  - offset: the number of bytes from origin where the position indicator
 *            should be placed
 *  - origin: origin position
 *            SEEK_SET: origin is the start of the file
 *            SEEK_CUR: origin is the current position
 *            SEEK_END: origin is the end of the stream
 * return value
 *  - 0 if successful, 1 otherwise
 * -------------------------------------------------------------------------- */
uint8_t fat32_fseek(MFILE *fp, uint32_t offset, uint8_t origin)
{   
    // Not impelemented yet
    
    switch(origin) {
        case SEEK_SET:
        break;
        
        case SEEK_CUR:
        break;
        
        case SEEK_END:
        break;
        
        default:
            return -1;
    }
        
    return -1;
}


/* ----------------------------------------------------------------------------
 * read a character from a file and increment the file position indicator by 1
 * arguments:
 *  - fp: file pointer
 * return value
 *  - 1-byte value read from the file ((-1) EOF in case of end of file)
 * -------------------------------------------------------------------------- */
int8_t fat32_fgetc(MFILE *fp)
{
    uint16_t buffer_offset;
    uint32_t cluster;

    if(fp->pos_indicator >= fp->size) {
        return EOF;             // end of file
    }

    buffer_offset = fp->pos_indicator&0x1ff;
    if(buffer_offset == 0) {
        mmc_read_block(fp->current_sector, fp->buffer);
        if(fat32_sector2cluster(fp->current_sector) == fat32_sector2cluster((fp->current_sector)+1))
            fp->current_sector++;

        else {
            cluster = fat32_find_next_cluster(fat32_sector2cluster(fp->current_sector));
            fp->current_sector = fat32_cluster2sector(cluster);
        }
    }

    fp->pos_indicator++;

    return fp->buffer[buffer_offset];
}

/* ----------------------------------------------------------------------------
 * load the file buffer with a block from the file
 * and sets the file position indicator to the byte past the last byte that has been read
 * 파일 포인터가 가리키는 파일에서 한 블럭을 읽어서 파일 버퍼를 채운다.
 * 그리고 읽어온 데이터의 길이에 맞게 파일 위치 인디케이터를 수정한다.
 * 파일 위치 인디케이터는 파일 내에서 다음에 읽어올 곳을 가리킨다.
 *
 * This function is slightly different from the standard one
 * intended for reducing the required memory
 *
 * arguments:
 *  - fp: file pointer
 * return value
 *  - the length of data read from the file
 * -------------------------------------------------------------------------- */
uint16_t fat32_fread(MFILE *fp)
{
    uint32_t cluster;

    if(fp->pos_indicator >= fp->size) {
        return 0;       // end of file
    }

    mmc_read_block(fp->current_sector, fp->buffer);
    fp->pos_indicator += BYTES_PER_SECTOR;

    if(fp->pos_indicator >= fp->size)
        return (BYTES_PER_SECTOR - (fp->pos_indicator-fp->size));

    if(fat32_sector2cluster(fp->current_sector) == fat32_sector2cluster((fp->current_sector)+1)) {
        fp->current_sector++;
    }
    else {
        cluster = fat32_find_next_cluster(fat32_sector2cluster(fp->current_sector));
        fp->current_sector = fat32_cluster2sector(cluster);
    }

    return BYTES_PER_SECTOR;
}

/* ----------------------------------------------------------------------------
 * get volume label
 * arguments:
 *  - volume_label: pointer to the buffer where the name is to be stored
 *    the length of the buffer should be >= 12
 * -------------------------------------------------------------------------- */
void fat32_get_volume_label(int8_t *volume_label)
{

    uint8_t i, l;
    uint32_t sector, cluster;
    DIRECTORY_ENTRY *dir_entry;

    cluster = fat32_get_root_cluster();
    sector = fat32_cluster2sector(cluster);
    
    while(1) {
        for(l=0; l<fat32_get_sectors_per_cluster(); l++) {
            mmc_read_block(sector, mmc_buffer);
            dir_entry = (DIRECTORY_ENTRY *)mmc_buffer;

            for(i=0; i<(BYTES_PER_SECTOR/32); i++) {
                if(dir_entry->deName[0] == FILE_HEADER_EMPTY) {
                    // there is no more directory entries
                    volume_label[0] = '\0';
                    return;      
                }  
                
                if(dir_entry->deName[0] != FILE_HEADER_DELETED) {
                    if(dir_entry->deAttributes != FILE_ATTR_LONG_FILENAME) {
                        if(dir_entry->deAttributes & FILE_ATTR_VOLUME) {
                            memcpy(volume_label, &dir_entry->deName[0], 11);
                            volume_label[11] = '\0';
                            return;
                        }
                    }
                }
                dir_entry++;
            }
            sector++;
        }
        cluster = fat32_find_next_cluster(cluster);
    }
}

/* ----------------------------------------------------------------------------
 * show directories and files in a cluster
 * arguments:
 *  - cluster: cluster number
 *
 * if this function is not used, comment out SHOW_DIRECTORY to reduce RAM usage
 * -------------------------------------------------------------------------- */
#ifdef SHOW_DIRECTORY
void fat32_show_directory(uint32_t cluster)
{
    #define DT_2SECONDS_MASK        0x1F    // seconds divided by 2
    #define DT_2SECONDS_SHIFT       1       // SHIFT to right, ALL the others are to left
    #define DT_MINUTES_MASK         0x7E0   // minutes
    #define DT_MINUTES_SHIFT        5
    #define DT_HOURS_MASK           0xF800  // hours
    #define DT_HOURS_SHIFT          11

    #define DD_DAY_MASK             0x1F    // day of month
    #define DD_DAY_SHIFT            0
    #define DD_MONTH_MASK           0x1E0   // month
    #define DD_MONTH_SHIFT          5
    #define DD_YEAR_MASK            0xFE00  // year - 1980
    #define DD_YEAR_SHIFT           9

    int8_t filename[LONG_FILE_NAME_BUF_LEN];
    uint8_t day, month, year, hour, minutes, seconds;
    uint16_t time, date, num_dirs, num_files;
    uint32_t de_physical_addr;
    DIRECTORY_ENTRY de;

    printf("\n\r ");
    num_dirs = num_files = 0;
    
    de_physical_addr = fat32_get_first_file_info(cluster, filename, &de);
    while(de_physical_addr) {
        //printf("attr:%02x ", de.deAttributes);
        //printf("cluster:%07lx ", ((uint32_t)de.deHighClust<<16) + de.deStartCluster);
        date= de.deCDate[0] + (de.deCDate[1] << 8);
        time= de.deCTime[0] + (de.deCTime[1] << 8);
        day=(date&DD_DAY_MASK)>>DD_DAY_SHIFT;
        month=(date&DD_MONTH_MASK)>>DD_MONTH_SHIFT;
        year=(date&DD_YEAR_MASK)>>DD_YEAR_SHIFT;
        hour=(time&DT_HOURS_MASK)>>DT_HOURS_SHIFT;
        minutes=(time&DT_MINUTES_MASK)>>DT_MINUTES_SHIFT;
        seconds=(time&DT_2SECONDS_MASK)<<DT_2SECONDS_SHIFT;
        printf("%04d/%02d/%02d ", year+1980, month, day);
        printf("%02d:%02d:%02d ", hour, minutes, seconds);
        if(de.deAttributes & FILE_ATTR_DIRECTORY) {
            num_dirs++;
            printf("<DIR>       ");
        }
        else {
            num_files++;
            printf("%11ld ", de.deFileSize);        
        }
        printf("%s\r\n ", filename);
        de_physical_addr = fat32_get_next_file_info(de_physical_addr, filename, &de);
    }
    printf("\n\r");
    printf("\n\r%32d FILES \n\r%32d DIRECTORIES\n\r", num_files, num_dirs);
}
#endif

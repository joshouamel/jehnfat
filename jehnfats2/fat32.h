/*
 * ----------------------------------------------------------------------------
 *          Header file for FAT32 file system library
 *                              written by Jungho Moon
 * ----------------------------------------------------------------------------
 */

#ifndef FAT32_H
#define FAT32_H

// some useful cluster numbers
#define MSDOSFSROOT     0               // cluster 0 means the root dir
#define CLUST_FREE      0               // cluster 0 also means a free cluster
#define MSDOSFSFREE     CLUST_FREE
#define CLUST_FIRST     2               // first legal cluster number
#define CLUST_RSRVD     0xfffffff6      // reserved cluster range
#define CLUST_BAD       0xfffffff7      // a cluster with a defect
#define CLUST_EOFS      0xfffffff8      // start of eof cluster range
#define CLUST_EOFE      0xffffffff      // end of eof cluster range

#define FAT12_MASK      0x00000fff      // mask for 12 bit cluster numbers
#define FAT16_MASK      0x0000ffff      // mask for 16 bit cluster numbers
#define FAT32_MASK      0x0fffffff      // mask for FAT32 cluster numbers

// partition type used in the partition record
#define PART_TYPE_UNKNOWN       0x00
#define PART_TYPE_FAT12         0x01
#define PART_TYPE_XENIX         0x02
#define PART_TYPE_DOSFAT16      0x04
#define PART_TYPE_EXTDOS        0x05
#define PART_TYPE_FAT16         0x06
#define PART_TYPE_NTFS          0x07
#define PART_TYPE_FAT32         0x0B
#define PART_TYPE_FAT32LBA      0x0C
#define PART_TYPE_FAT16LBA      0x0E
#define PART_TYPE_EXTDOSLBA     0x0F
#define PART_TYPE_ONTRACK       0x33
#define PART_TYPE_NOVELL        0x40
#define PART_TYPE_PCIX          0x4B
#define PART_TYPE_PHOENIXSAVE   0xA0
#define PART_TYPE_CPM           0xDB
#define PART_TYPE_DBFS          0xE0
#define PART_TYPE_BBT           0xFF

// file attributes
#define FILE_ATTR_READONLY      0x01    // file is readonly
#define FILE_ATTR_HIDDEN        0x02    // file is hidden
#define FILE_ATTR_SYSTEM        0x04    // file is a system file
#define FILE_ATTR_VOLUME        0x08    // entry is a volume label
#define FILE_ATTR_LONG_FILENAME 0x0f    // this is a long filename entry
#define FILE_ATTR_DIRECTORY     0x10    // entry is a directory name
#define FILE_ATTR_ARCHIVE       0x20    // file is new or modified

#define FILE_HEADER_EMPTY       0x00    // slot has never been used
#define FILE_HEADER_DELETED     0xe5    // file is deleted

// FAT32 data structures
#define BYTES_PER_SECTOR        512
#define LONG_FILE_NAME_MAX_LEN  45      // maximum number of characters
#define LONG_FILE_NAME_BUF_LEN  (2*(LONG_FILE_NAME_MAX_LEN)+2)

typedef struct
{
    uint8_t     mbrPartCode[512-64-2];  // (offset: 0x000)  padded so struct is 512b
    uint8_t     mbrPartEntry1[16];      // (offset: 0x1be)  partition entry 1 
    uint8_t     mbrPartEntry2[16];      // (offset: 0x1ce)  partition entry 2
    uint8_t     mbrPartEntry3[16];      // (offset: 0x1de)  partition entry 3
    uint8_t     mbrPartEntry4[16];      // (offset: 0x1ee)  partition entry 4
    uint8_t     mbrBootSectSig0;        // (offset: 0x1fe)  value: 0x55
    uint8_t     mbrBootSectSig1;        // (offset: 0x1ff)  value: 0xaa
} MASTER_BOOT_RECORD;

typedef struct                          
{           
    uint8_t     peIsActive;             // (offset: 0x00)   0x80 indicates active partition
    uint8_t     peStartHead;            // (offset: 0x01)   starting head for partition
    uint16_t    peStartCylSect;         // (offset: 0x02)   starting cylinder and sector
    uint8_t     pePartType;             // (offset: 0x04)   partition type 
    uint8_t     peEndHead;              // (offset: 0x05)   ending head for this partition
    uint16_t    peEndCylSect;           // (offset: 0x06)   ending cylinder and sector
    uint32_t    peStartLBA;             // (offset: 0x08)   first LBA sector for this partition
    uint32_t    peSize;                 // (offset: 0x0c)   size of this partition (number of sectors)
} PARTION_ENTRY;

typedef struct                          
{           
    uint8_t     bsJump[3];              // (offset: 0x00)   jump instruction + NOP
    int8_t      bsOEMName[8];           // (offset: 0x03)   OEM name and version

    // BIOS parameter block
    uint16_t    bsBytesPerSec;          // (offset: 0x0b)   bytes per sector
    uint8_t     bsSecPerClust;          // (offset: 0x0d)   sectors per cluster
    uint16_t    bsResSectors;           // (offset: 0x0e)   number of reserved sectors
    uint8_t     bsFATs;                 // (offset: 0x10)   number of FATs
    uint16_t    bsRootDirEnts;          // (offset: 0x11)   maximum number of root directory entries (N/A for FAT32)
    uint16_t    bsSectors;              // (offset: 0x13)   total number of sectors (N/A for FAT32)
    uint8_t     bsMedia;                // (offset: 0x15)   media descriptor
    uint16_t    bsFATsecs;              // (offset: 0x16)   number of sectors per FAT (N/A for FAT32)
    uint16_t    bsSecPerTrack;          // (offset: 0x18)   sectors per track
    uint16_t    bsHeads;                // (offset: 0x1a)   number of heads
    uint32_t    bsHiddenSecs;           // (offset: 0x1c)   # of hidden sectors
    uint32_t    bsHugeSectors;          // (offset: 0x20)   # of sectors if bsSectors == 0
    uint32_t    bsBigFATsecs;           // (offset: 0x24)   like bsFATsecs for FAT32
    uint16_t    bsExtFlags;             // (offset: 0x28)   extended flags:
    uint16_t    bsFSVers;               // (offset: 0x2a)   filesystem version
    uint32_t    bsRootClust;            // (offset: 0x2c)   start cluster for root directory
    uint16_t    bsFSInfo;               // (offset: 0x30)   filesystem info structure sector
    uint16_t    bsBackup;               // (offset: 0x32)   backup boot sector
    uint8_t     bsReserved[12];         // (offset: 0x34)   reserved
    
    uint8_t     bsDriveNumber;          // (offset: 0x40)   logical drive number
    uint8_t     bsReserved1;            // (offset: 0x41)   unused
    uint8_t     bsBootSignature;        // (offset: 0x42)   extended boot signature (0x29)
    uint32_t    bsVolumeID;             // (offset: 0x43)   serial number of partition
    int8_t      bsVolumeLabel[11];      // (offset: 0x47)   volume name of partition
    int8_t      bsFileSystemType[8];    // (offset: 0x52)   file system type
} BOOT_SECTOR;

typedef struct {
    uint8_t     deName[8];              // (offset: 0x00)   filename, blank filled
    uint8_t     deExtension[3];         // (offset: 0x08)   extension, blank filled
    uint8_t     deAttributes;           // (offset: 0x0b)   file attributes
    uint8_t     deLowerCase;            // (offset: 0x0c)   NT VFAT lower case flags
    uint8_t     deCHundredth;           // (offset: 0x0d)   hundredth of seconds in CTime
    uint8_t     deCTime[2];             // (offset: 0x0e)   create time
    uint8_t     deCDate[2];             // (offset: 0x10)   create date
    uint8_t     deADate[2];             // (offset: 0x12)   access date
    uint16_t    deHighClust;            // (offset: 0x14)   high bytes of cluster number
    uint8_t     deMTime[2];             // (offset: 0x16)   last update time
    uint8_t     deMDate[2];             // (offset: 0x18)   last update date
    uint16_t    deStartCluster;         // (offset: 0x1a)   starting cluster of file
    uint32_t    deFileSize;             // (offset: 0x1c)   size of file in bytes
} DIRECTORY_ENTRY;

typedef struct {
    uint16_t sectors_per_cluster;
    uint32_t root_cluster;
    uint32_t sectors_per_fat;
    uint32_t first_fat_sector;
    uint32_t first_data_sector;
} FAT32_INFO;    

typedef struct {
    uint32_t size;                  // file size
    uint32_t start_cluster;         // start cluster
    uint32_t current_sector;        // current sector
    uint32_t pos_indicator;         // position indicator
    uint8_t  status;                // status flags
    uint8_t  buffer[BYTES_PER_SECTOR];
} MFILE; 

// function prototypes
int8_t fat32_init(void);
uint32_t fat32_get_current_dir_cluster(void);
int8_t fat32_get_dir_entry(uint32_t, int8_t *, DIRECTORY_ENTRY *);
uint32_t fat32_get_first_file_info(uint32_t, int8_t *, DIRECTORY_ENTRY *);
uint32_t fat32_get_next_file_info(uint32_t, int8_t *, DIRECTORY_ENTRY *);
int8_t fat32_chdir(int8_t *);
MFILE *fat32_fopen(int8_t *);
uint8_t fat32_fclose(MFILE *);
void fat32_frewind(MFILE *);
uint8_t fat32_fseek(MFILE *, uint32_t, uint8_t);
int8_t fat32_fgetc(MFILE *);
uint16_t fat32_fread(MFILE *);
void fat32_get_volume_label(int8_t *);
void fat32_show_directory(uint32_t cluster);
#endif

#pragma once
#include "fatfs_typedef.h"
#define PACK(a) a
//-----------------------------------------------------------------------------
// FAT32 File Attributes and Types
//-----------------------------------------------------------------------------
enum FileAttr : uint8 {
	ReadOnly = 0x01,
	Hidden = 0x02,
	System =0x04,
	VolumeID=0x08,
	LFN_TEXT = 0xf,
	Directory=0x10,
	Archive=0x20,
};
PACK(
struct fat_dir_entry
{
	uint8 Name[11];
	FileAttr Attr;
	uint8 NTRes;
	uint8 CrtTimeTenth;
	uint8 CrtTime[2];
	uint8 CrtDate[2];
	uint8 LstAccDate[2];
	uint16 FstClusHI;
	uint8 WrtTime[2];
	uint8 WrtDate[2];
	uint16 FstClusLO;
	uint32 FileSize;
});
PACK(
	struct fat_dir_lfn_entry
{
	uint8 order;
	uint8 name0[10];
	FileAttr Attr;
	uint8 NTRes;
	uint8 checksum;
	uint8 name1[12];
	uint16 FstClusLO;
	uint8 name2[4];
});

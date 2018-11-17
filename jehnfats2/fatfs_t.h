#pragma once
#include <type_traits>
#include <vector>
#include "fatfs_typedef.h"

typedef enum eFatType
{
	FAT_TYPE_16,
	FAT_TYPE_32
} tFatType;
typedef void fatfsCallback(void*, int32);

struct fatfs_t {
	// Filesystem globals
	uint8                   sectors_per_cluster;
	uint32                  cluster_begin_lba;
	uint32                  rootdir_first_cluster;
	uint32                  rootdir_first_sector;
	uint32                  rootdir_sectors;
	uint32                  fat_begin_lba;
	uint16                  fs_info_sector;
	uint32                  lba_begin;
	uint32                  fat_sectors;
	uint32                  next_free_cluster;
	uint16                  root_entry_count;
	uint16                  reserved_sectors;
	uint8                   num_of_fats;
	tFatType                fat_type;
};
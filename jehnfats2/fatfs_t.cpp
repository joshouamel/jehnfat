#include "fatfs_t.h"
#include "fatdir_t.h"

void fatfs_t::sectorRead(uint32 start_cluster, uint32 offset, uint8 * target, fatfsCallback *, void *)
{
	uint32 sector_to_read = 0;
	uint32 cluster_to_read = 0;
	uint32 cluster_chain = 0;
	uint32 i;
	uint32 lba;

	// FAT16 Root directory
	if (fs->fat_type == FAT_TYPE_16 && start_cluster == 0)
	{
		if (offset < fs->rootdir_sectors)
			lba = fs->lba_begin + fs->rootdir_first_sector + offset;
		else
			return 0;
	}
	// FAT16/32 Other
	else
	{
		// Set start of cluster chain to initial value
		cluster_chain = start_cluster;

		// Find parameters
		cluster_to_read = offset / fs->sectors_per_cluster;
		sector_to_read = offset - (cluster_to_read*fs->sectors_per_cluster);

		// Follow chain to find cluster to read
		for (i = 0; i < cluster_to_read; i++)
			cluster_chain = fatfs_find_next_cluster(fs, cluster_chain);

		// If end of cluster chain then return false
		if (cluster_chain == FAT32_LAST_CLUSTER)
			return 0;

		// Calculate sector address
		lba = fatfs_lba_of_cluster(fs, cluster_chain) + sector_to_read;
	}

	// User provided target array
	if (target)
		return fs->disk_io.read_media(lba, target, 1);
	// Else read sector if not already loaded
	else if (lba != fs->currentsector.address)
	{
		fs->currentsector.address = lba;
		return fs->disk_io.read_media(fs->currentsector.address, fs->currentsector.sector, 1);
	}
	else
		return 1;
}


struct lfn_cache {
	uint8 string[0x40][26];
	uint8 of;
};
//-------------------------------------------------------------
// fatfs_get_file_entry: Find the file entry for a filename
//-------------------------------------------------------------
bool lfnwriter(lfn_cache* ch, fat_dir_lfn_entry* etr) {
	if (ch->of != 0 && (etr->order & 0x40)) {
		return false;
	}
	else if (ch->of == 0 && !((etr->order & 0x40))) {
		return false;
	}
	if (ch->of == 0) {
		ch->of = etr->order & 0x3F;
	}
	auto d=ch->string[((etr->order & 0x3F) - 1)];
	for (int i = 0; i < 10; i++) {
		d[i] = etr->name0[i];
	}
	for (int i = 0; i < 12; i++) {
		d[i+14] = etr->name1[i];
	}
	for (int i = 0; i < 4; i++) {
		d[i+28] = etr->name0[i];
	}
}
struct sghtlgjlk {
	int hndl;
	fatfs_t *fs;
	lfn_cache lfn;
	int idx;
	fat_dir_entry fd;
	uint8* cpr;
};
#define FATFS_INC_LFN_SUPPORT 1
#define FILE_HEADER_BLANK 0
#define FILE_HEADER_DELETED 0xE5
void asdfdnmfgkjfgjk(void* ntp,int sz) {
	sghtlgjlk *nt = (sghtlgjlk*)ntp;
	if (sz == 0) {
		nt
	}
#if FATFS_INC_LFN_SUPPORT
	// Long File Name Text Found
	if (nt->fd.Attr&&LFN_TEXT)
	{
		if (!lfnwriter(nt->lfn, nt->fd)) {
			//오류!
			return;
		} 
	}

	// If Invalid record found delete any long file name information collated
	else if ((nt->fd.Name[0] == FILE_HEADER_BLANK) ||
		(nt->fd.Name[0] == FILE_HEADER_DELETED) ||
		(nt->fd.Attr == VolumeID) ||
		(nt->fd.Attr & (System | Hidden))) {
		nt->lfn.of = 0;
	}

	// Normal SFN Entry and Long text exists
	else if ((nt->fd.Attr != LFN_TEXT) &&
		(nt->fd.Name[0] != FILE_HEADER_BLANK) &&
		(nt->fd.Name[0] != FILE_HEADER_DELETED) &&
		(nt->fd.Attr != VolumeID) &&
		(!(nt->fd.Attr&(System|Hidden))) &&
		(nt->lfn.of!=0))
	{
		long_filename = fatfs_lfn_cache_get(&lfn);

		// Compare names to see if they match
		if (fatfs_compare_names(long_filename, name_to_find))
		{
			memcpy(sfEntry, directoryEntry, sizeof(struct fat_dir_entry));
			return 1;
		}

		fatfs_lfn_cache_init(&lfn, 0);
	}
	else
#endif
		// Normal Entry, only 8.3 Text
		if (fatfs_entry_sfn_only(directoryEntry))
		{
			memset(short_filename, 0, sizeof(short_filename));

			// Copy name to string
			for (i = 0; i < 8; i++)
				short_filename[i] = directoryEntry->Name[i];

			// Extension
			dotRequired = 0;
			for (i = 8; i < 11; i++)
			{
				short_filename[i + 1] = directoryEntry->Name[i];
				if (directoryEntry->Name[i] != ' ')
					dotRequired = 1;
			}

			// Dot only required if extension present
			if (dotRequired)
			{
				// If not . or .. entry
				if (short_filename[0] != '.')
					short_filename[8] = '.';
				else
					short_filename[8] = ' ';
			}
			else
				short_filename[8] = ' ';

			// Compare names to see if they match
			if (fatfs_compare_names(short_filename, name_to_find))
			{
				memcpy(sfEntry, directoryEntry, sizeof(struct fat_dir_entry));
				return 1;
			}

			fatfs_lfn_cache_init(&lfn, 0);
		}
}
//nt->fs->vread(dest,handle,offset,sz,callback)
void gegddfsfsdasfdasfdasfdafsd(void* fs, int hndl) {
	sghtlgjlk *nt=(sghtlgjlk*)fs;
	//초기화
	nt->lfn.of = 0;
	nt->idx=0;
	nt->hndl = hndl;
	nt->fs->vread(&nt->fd,hndl,nt->idx,sizeof(nt->fd),nt,&asdfdnmfgkjfgjk);
}
uint32 fatfs_get_file_entry(struct fatfs_t *fs, uint32 Cluster, char *name_to_find)
{
	uint8 item = 0;
	uint16 recordoffset = 0;
	uint8 i = 0;
	int x = 0;
	char *long_filename = NULL;
	char short_filename[13];
	int dotRequired = 0;
	struct fat_dir_entry *directoryEntry;
	sghtlgjlk *nt = new sghtlgjlk;
	nt->fs = fs;
	nt->cpr = name_to_find;
	fs->getClusterHandle(Cluster, nt,&gegddfsfsdasfdasfdasfdafsd);

	return 0;
}
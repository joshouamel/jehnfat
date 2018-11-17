#include "fileHandle0.h"
#include <algorithm>

void fileHandle0::read(uint32 cluster, uint32 offset, void* arg, getaddrandintf f)
{
	argu* argd = new argu{this,cluster,offset,arg,f};
	vread(argd);
	return;
}
void fileHandle0::vread(void * dfd)
{
	argu* arg = (argu*)dfd;
	if (arg->offset != 0) {
		if (arg->_this->fs.fat_type == FAT_TYPE_32) {
			if(arg->cluster==){
				auto f = arg->callback;
				auto argc = arg->arg;
				delete arg;
				f(argc, nullptr, arg->cluster);
				return;
			}
			uint32 dfd = (arg->_this->fs.sectors_per_cluster * 512 / 4);
			uint32 bloc = arg->cluster / dfd;
			uint32 targ = arg->_this->fs.fat_begin_lba + bloc * arg->_this->fs.sectors_per_cluster;
			arg->_this->da->getaddr(targ, arg, load);
			return;
		}
	}
	else {
		auto dest=arg->_this->fs.rootdir_first_sector + (arg->cluster - arg->_this->fs.rootdir_first_cluster)* arg->_this->fs.sectors_per_cluster;
		arg->_this->da->getaddr( dest, arg, loaded);
	}
}
void fileHandle0::load(void* dfd, void* dest) {
	argu* arg = (argu*)dfd;
	if (arg->_this->fs.fat_type == FAT_TYPE_32) {
		uint32 dfd = (arg->_this->fs.sectors_per_cluster * 512 / 4);
		uint32 pos = arg->cluster % dfd;
		arg->cluster =((uint32*)dest)[pos];
		arg->offset--;
		vread(arg);
		return;
	}
}
void fileHandle0::loaded(void* dfd, void* cb) {
	argu* arg = (argu*)dfd;
	auto cls = arg->cluster;
	auto argc = arg->arg;
	auto f = arg->callback;
	delete arg;
	f(argc, cb, cls);
}



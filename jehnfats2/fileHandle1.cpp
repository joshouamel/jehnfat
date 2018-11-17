#include "fileHandle1.h"
#include <algorithm>

void fileHandle1::getClusterHandle(uint32 cluster, void *b, callback* c)
{
	auto it = ctoh.find(cluster);
	if (it == ctoh.end()) {
		auto hndl = htable.alloc(cluster);
		c(b, hndl);
		return;
	}
	c(b, it->second);
	return;
}

void fileHandle1::read(void * dest, uint32 handle, uint32 offset, uint32 size, void* arg, diskAllocator::getintf cb)
{
	if (handlecnt.find(handle) != handlecnt.end()) {

	}
}
void fileHandle1::vread(void* dfd) {
	argu* arg = (argu*)dfd;
	if (arg->size == 0) {
		auto f = arg->callback;
		auto argc = arg->arg;
		auto ts = arg->tsize;
		delete arg;
		f(argc, ts);
		return;
	}
	arg->_this->da->read(arg->handle, arg->offset / 512 / arg->_this->da->fs.sectors_per_cluster, arg, load);
	return;
}
void fileHandle1::load(void* dfd, void* des, uint32 cluster) {
	argu* arg = (argu*)dfd;
	if (des == nullptr) {
		//더이상 읽을 수 없음...
		arg->size = 0;
		vread(arg);
		return;
	}
	auto bgd = arg->offset % (512 * arg->_this->da->fs.sectors_per_cluster);
	auto ts = (512 * arg->_this->da->fs.sectors_per_cluster) - bgd;
	if (ts > arg->size)ts = arg->size;
	memcpy(arg->dest, ((char*)des) + bgd, ts);
	arg->offset += ts;
	arg->tsize += ts;
	arg->size -= ts;
	arg->dest = ((char*)arg->dest) + ts;
	vread(arg);
	return;
}

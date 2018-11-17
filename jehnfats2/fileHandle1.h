#pragma once
#pragma once
#include "fatfs_t.h"
#include "fatfs_typedef.h"
#include "handler.h"
#include <map>
#include <list>
#include "fileHandle0.h"
class fileHandle1 {
public:
	void getClusterHandle(uint32 cluster, void*, diskAllocator::getintf*);
	void freeClusterHandle(uint32 cluster, void*, diskAllocator::getintf*);
	void read(void* dest, uint32 cluster, uint32 offset, uint32 size, void*, diskAllocator::getintf);
private:
	std::map<uint32, uint32> handlecnt;
	fileHandle0 *da;
	struct argu {
		fileHandle1* _this;
		void* dest;
		uint32 handle;
		uint32 offset;
		uint32 size;
		uint32 tsize;
		void * arg;
		diskAllocator::getintf* callback;
	};
	static void vread(void*);
	static void load(void* arg, void*addr, uint32 cls);
};
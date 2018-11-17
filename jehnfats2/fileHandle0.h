#pragma once
#pragma once
#include "fatfs_t.h"
#include "fatfs_typedef.h"
#include "handler.h"
#include <map>
#include <list>
#include "mediaAccess.h"
class fileHandle0 {
public:

	typedef void getaddrandintf(void* arg,void* addr, uint32 req);
	void read(uint32 cluster, uint32 offset, void*, getaddrandintf);
	fatfs_t fs;
	diskAllocator *da;
private:
	struct argu {
		fileHandle0* _this;
		uint32 cluster;
		uint32 offset;
		void * arg;
		getaddrandintf* callback;
	};
	static void vread(void*);
	static void loaded(void*, void*);
	static void load( void*, void*);
};
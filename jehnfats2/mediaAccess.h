#pragma once
#include "fatfs_typedef.h"
#include "handler.h"
#include <map>
#include <list>
class diskAllocator {
public:
	typedef void getaddrf(void* arg, void* req);
	typedef void getintf(void* arg, uint32 req);
	inline diskAllocator(uint32 sectorPerCluster,void* acc,uint32 size):sectorPerCluster(sectorPerCluster){}
	void getaddr(uint32 sector, void* arg, getaddrf* f);
private:
	struct getaddr_argt {
		diskAllocator* _this;
		uint32 cluster;
		void* arg;
		getaddrf* f;
,	};
	static void getaddr_sub(void*, uint32);
	void allocpag(void* arg,getintf*);
	void read(void* dest, uint32 sector, uint32 cnt, void*, getintf s);
	void write(void* dest, uint32 sector, uint32 cnt, void*, getintf s);
	struct allocpag_sub_t {
		diskAllocator* _this;
		uint32 jjp;
		void * arg;
		getintf * f;
	};
	static void allocpag_sub0(void*, uint32);
	// Disk/Media API
	struct disk_if          disk_io;
	void* acc;
	uint32 size;
	std::map<uint32, uint32> pos2add;
	Handler<uint32, std::pair<uint32, uint32>> add2pos;
	std::list<uint32> Jp;
	uint32 sectorPerCluster;
};
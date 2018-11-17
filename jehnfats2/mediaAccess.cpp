#include "mediaAccess.h"
#include <algorithm>
void diskAllocator::getaddr(uint32 sector, void * arg, getaddrf * f) {
	auto it = pos2add.find(sector);
	//�ȿö�� ����
	if (it == pos2add.end()) {
		//�÷��ߵ�
		getaddr_argt* argt = new getaddr_argt{ this,sector,arg,f };
		allocpag(argt, getaddr_sub);
		return;
	}
	else {
		f(arg, ((char*)acc)+it->first*sectorPerCluster*512);
		return;
	}
}
void diskAllocator::getaddr_sub(void* arggb, uint32 j) {
	getaddr_argt* arg = (getaddr_argt*)arggb;
	if (j== arg->_this->add2pos.null) {
		//�̰� ������;;
		arg->_this -> allocpag(arg, getaddr_sub);
	}
	else {
		arg->_this->add2pos.v[j] = std::make_pair(arg->cluster, 0);
		arg->_this->add2pos.current = j + 1;
		if (arg->_this->add2pos.current >= arg->_this->add2pos.max)	
			arg->_this->add2pos.current = arg->_this->add2pos.min;
		arg->_this->Jp.push_back(j);
		arg->_this->pos2add[arg->cluster] = j;
		auto f = arg->f;
		auto gg = arg->arg;
		auto des = ((char*)arg->_this->acc) + j * arg->_this->sectorPerCluster * 512;
		delete arg;
		f(gg, des);
		return;
	}
}
void diskAllocator::allocpag(void * arg, getintf * f){
	void* dest;
	uint32 hndl=add2pos.alloc();
	if (hndl == add2pos.null) {
		//�i�Ƴ��ߵ�.
		auto jjp = Jp.front();
		//��Ƽ�� 0�̸� ����
		if (add2pos.v[jjp].second == 0) {
			pos2add.erase(add2pos.v[jjp].first);
			add2pos.v.erase(jjp);
			add2pos.current = jjp;
			f(arg,jjp);
			return;
		}
		else {
			auto aggg = new allocpag_sub_t{ this,jjp,arg,f};
			write(((char*)acc) + jjp * sectorPerCluster * 512, add2pos.v[jjp].first, sectorPerCluster,aggg, allocpag_sub0);
			return;
		}
	}

	
}

 void  diskAllocator::allocpag_sub0(void * arglo, uint32 i)
{
	allocpag_sub_t* arg = (allocpag_sub_t * )arglo;
	if (i) {
		arg->_this->pos2add.erase(arg->_this->add2pos.v[arg->jjp].first);
		arg->_this->add2pos.v.erase(arg->jjp);
		arg->f(arg->arg, arg->jjp);
		return;

	}
	else {//�ϴ� ������ ���� ���ϴ°ɷ�...
		arg->f(arg->arg, arg->_this->add2pos.null);
		return;
	}
}

#pragma once
#include "fileHandle1.h"
class fileHandle2 {
public:
	enum om {
		//없으면 파일을 생성한다. 있으면 생성하지 않고 리턴
		Creat = 0x01,
		//없으면 파일을 생성, 있으면 에러
		Excl = 0x02,
		//read때 없으면 그냥 0리턴하겠다
		NonBlock = 0x04,
		//프로세스 독점권
		NonInherit = 0x08,
		//읽기 모드
		Read = 0x10,
		//쓰기모드
		Write = 0x20,
		//없애버려
		Trunk = 0
	};
	enum oc {
		//액세스 거부
		AccessDenied=0x1,
		//파일이 존재하지 않음
		FileNotExist,
		//읽기전용
		ReadOnly,
		//리소스 부족
		SystemResourceLeak,
		//프로그램 점검 필요
		ProgramLeak,
		//파일 이름이 길다
		PathnameTooLong,
		//알수 없는 파일 이름
		UnkownPath

	};
	void open(char* s,om o);

	fileHandle1* da;
private:
	std::map<uint32,uint32> stddsf;

};
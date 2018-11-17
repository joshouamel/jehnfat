#pragma once
#include "fileHandle1.h"
class fileHandle2 {
public:
	enum om {
		//������ ������ �����Ѵ�. ������ �������� �ʰ� ����
		Creat = 0x01,
		//������ ������ ����, ������ ����
		Excl = 0x02,
		//read�� ������ �׳� 0�����ϰڴ�
		NonBlock = 0x04,
		//���μ��� ������
		NonInherit = 0x08,
		//�б� ���
		Read = 0x10,
		//������
		Write = 0x20,
		//���ֹ���
		Trunk = 0
	};
	enum oc {
		//�׼��� �ź�
		AccessDenied=0x1,
		//������ �������� ����
		FileNotExist,
		//�б�����
		ReadOnly,
		//���ҽ� ����
		SystemResourceLeak,
		//���α׷� ���� �ʿ�
		ProgramLeak,
		//���� �̸��� ���
		PathnameTooLong,
		//�˼� ���� ���� �̸�
		UnkownPath

	};
	void open(char* s,om o);

	fileHandle1* da;
private:
	std::map<uint32,uint32> stddsf;

};
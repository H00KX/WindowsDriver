// 2.��λ�������λ���Ȼ���.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

bool �Ƿ��Ǹ�λ����1(int i)
{
	
	if (*((char*)&i) == 1)
	{
		printf("��λ����\n");
		return false;
	}
	else
	{
		printf("��λ����\n");
		return true;
	}
}

int �ߵ�λ����(int nParam)
{
	return ((nParam >> 24) & 0xFF) | ((nParam >> 16) & 0xFF00) | ((nParam << 8) & 0xFF0000) | ((nParam << 24) & 0xFF000000);
}

int main()
{
	int i = 1;
	�Ƿ��Ǹ�λ����1(i);
	i = �ߵ�λ����(i);
	�Ƿ��Ǹ�λ����1(i);

	getchar();

    return 0;
}


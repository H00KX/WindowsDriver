// 1.�жϸ�λ���Ȼ��ǵ�λ����.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

bool �Ƿ��Ǹ�λ����1()
{
	int i = 0x1;
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

bool �ж��Ƿ��Ǹ�λ����2()
{
	union str {
		int i;
		char c;
	};

	str str;
	str.i = 1;

	if (str.c == 1)
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



int main()
{
	�Ƿ��Ǹ�λ����1();

	�ж��Ƿ��Ǹ�λ����2();

	getchar();

    return 0;
}


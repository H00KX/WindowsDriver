// 3.����һ����������״̬��1�ĸ���.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

int NumOfOne(int i)
{
	int num = 0;
	while (i)
	{
		++num;
		i = (i)&(i - 1);
	}
	return num;
}

int main()
{

	printf("1: %d\n",NumOfOne(1));

	printf("255: %d\n",NumOfOne(255));


	getchar();

    return 0;
}


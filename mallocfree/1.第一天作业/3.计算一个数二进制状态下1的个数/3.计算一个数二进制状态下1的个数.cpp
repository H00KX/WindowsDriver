// 3.计算一个数二进制状态下1的个数.cpp : 定义控制台应用程序的入口点。
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


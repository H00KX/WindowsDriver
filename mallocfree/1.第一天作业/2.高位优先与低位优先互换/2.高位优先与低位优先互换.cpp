// 2.高位优先与低位优先互换.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

bool 是否是高位优先1(int i)
{
	
	if (*((char*)&i) == 1)
	{
		printf("低位优先\n");
		return false;
	}
	else
	{
		printf("高位优先\n");
		return true;
	}
}

int 高低位互换(int nParam)
{
	return ((nParam >> 24) & 0xFF) | ((nParam >> 16) & 0xFF00) | ((nParam << 8) & 0xFF0000) | ((nParam << 24) & 0xFF000000);
}

int main()
{
	int i = 1;
	是否是高位优先1(i);
	i = 高低位互换(i);
	是否是高位优先1(i);

	getchar();

    return 0;
}


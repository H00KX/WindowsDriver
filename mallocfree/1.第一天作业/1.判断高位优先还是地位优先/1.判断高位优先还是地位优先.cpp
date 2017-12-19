// 1.判断高位优先还是地位优先.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

bool 是否是高位优先1()
{
	int i = 0x1;
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

bool 判断是否是高位优先2()
{
	union str {
		int i;
		char c;
	};

	str str;
	str.i = 1;

	if (str.c == 1)
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



int main()
{
	是否是高位优先1();

	判断是否是高位优先2();

	getchar();

    return 0;
}


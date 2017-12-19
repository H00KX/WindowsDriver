// 第二天作业.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

int my_strlen1(char* str)
{
	if (str == NULL || *str == '\0')
		return 0;
	char* p = str;
	int n = 0;
	while (*str++ != '\0')
		++n;
	return n;
}

void reserve_str1(char* str)
{
	int len = my_strlen1(str);

	for (int i = 0; i < len / 2; ++i)
	{
		char ch = str[i];
		str[i] = str[len - i - 1];
		str[len - i - 1] = ch;
	}
}

void reserve_str2(char* str, int nStart, int nNum)
{
	int len = my_strlen1(str);
	if (nNum == 0)
	{
		nNum = len - nStart;
	}

	for (int i = 0; i < nNum / 2; ++i)
	{
		char ch = str[nStart+i];
		str[nStart+i] = str[nStart + nNum - i - 1];
		str[nStart + nNum - i - 1] = ch;
	}
}



int main()
{
	char p[] = "Hello World!";

	//printf("%d\n", my_strlen1(p));
	
	//printf("%s\n", p);
	//reserve_str1(p);
	//printf("%s\n", p);


	reserve_str2(p, 0, 3);
	reserve_str2(p, 3, 0);
	reserve_str2(p, 0, 0);

	printf("%s\n", p);

	getchar();


    return 0;
}


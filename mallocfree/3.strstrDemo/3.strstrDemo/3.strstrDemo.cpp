// 3.strstrDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>

char * my_strstr(const char *str, char*substr)
{
	if (str == NULL || substr == NULL)
		return NULL;

	if (*substr == '\0')
		return (char*)str;

	char*cp = (char*)str;
	while (*cp != '\0')
	{
		char* s1 = cp;
		char* s2 = substr;
		while (*s1 && *s2 && !(*s1 - *s2))
		{
			s1++;
			s2++;
		}

		if (*s2 == '\0')
		{
			return cp;
		}

		cp++;
	}

	return NULL;
}


int main()
{
	strstr("hello,world", "hello");

    return 0;
}


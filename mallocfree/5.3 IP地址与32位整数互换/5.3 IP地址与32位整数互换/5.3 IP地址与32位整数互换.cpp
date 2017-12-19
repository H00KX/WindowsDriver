// 5.3 IP地址与32位整数互换.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


bool 是否是大尾()
{
	int i = 0x1;

	char ch = *(char*)&i;

	if (ch == 1)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void TurnEndian(int* i)
{
	*i = (((*i & 0xFF) << 24) | ((*i & 0xFF00) << 8) | ((*i & 0xFF0000) >> 8) | ((*i >> 24) & 0xFF));
}


void turnIp(char *szIp)
{
	int nIpArr[4] = { 0 };
	int nItems = 0;
	char *pPos1 = szIp;
	char *pPos2 = szIp;
	int nIpItem = 0;

	if (szIp == NULL || *szIp == '\0')
	{
		return;
	}


	while (1)
	{
		nIpItem = nIpItem * 10 + (*pPos2 - '0');

		++pPos2;


		if (*pPos2 == '.')
		{
			nIpArr[nItems++] = nIpItem;
			nIpItem = 0;
			++pPos2;
			continue;
		}

		if (*pPos2 == '\0')
		{
			nIpArr[nItems] = nIpItem;
			break;
		}

	}

	
	printf("%02X%02X%02X%02X\n", nIpArr[0], nIpArr[1], nIpArr[2], nIpArr[3]);
	

	if (!是否是大尾())
	{
		for (int i = 0; i < 4; ++i)
			TurnEndian(&nIpArr[i]);
	}



}

void turnAddToIp(unsigned int ip)
{
	char szIp[20] = { 0 };

	sprintf_s(szIp, "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

	printf("ip: %s\n", szIp);

}

int main()
{
	//turnIp("255.255.255.255");
    
	turnAddToIp(0xFFFFFFFF);
	return 0;
}


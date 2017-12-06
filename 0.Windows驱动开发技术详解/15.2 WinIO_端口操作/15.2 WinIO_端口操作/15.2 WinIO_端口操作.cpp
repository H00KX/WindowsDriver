// 15.2 WinIO_端口操作.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>

#include ".\\WinIoLib\\WinIo.h"

#pragma comment(lib,".\\WinIoLib\\WinIo.lib")

int main()
{
	bool bRet = InitializeWinIo();

	if (bRet)
	{
		printf("Load Driver successfully!\n");

		SetPortVal(0x378, 0, 1);

		ShutdownWinIo();
	}

	getchar();


    return 0;
}


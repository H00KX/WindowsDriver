// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>

int main()
{
	HANDLE hDevice = CreateFile(
		_T("\\\\.\\HelloDDK"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Open Device failed!\n");
		return 1;
	}

	OVERLAPPED overlap1 = { 0 };
	OVERLAPPED overlap2 = { 0 };

	UCHAR buffer[10];
	ULONG ulRead;

	BOOL bRead = ReadFile(hDevice, buffer, 10, &ulRead, &overlap1);
	if (!bRead && GetLastError() == ERROR_IO_PENDING)
	{
		printf("The operation is pending\n");
	}

	bRead = ReadFile(hDevice, buffer, 10, &ulRead, &overlap2);
	if (!bRead && GetLastError() == ERROR_IO_PENDING)
	{
		printf("The operation is pending\n");
	}

	//迫使程序终止2秒
	Sleep(2000);

	//创建IRP_MJ_CLEANUP IRP
	CloseHandle(hDevice);

    return 0;
}


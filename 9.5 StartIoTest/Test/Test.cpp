// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>

UINT WINAPI Thread1(LPVOID context)
{
	printf("Enter Thread\n");
	//等待5秒
	OVERLAPPED overlap = { 0 };
	overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	UCHAR buffer[10];
	ULONG ulRead;

	BOOL bRead = ReadFile(*(PHANDLE)context, buffer, 10, &ulRead, &overlap);

	//可以试验取消例程
	WaitForSingleObject(overlap.hEvent, INFINITE);
	return 0;
}

int main()
{
	HANDLE hDevice = CreateFile(_T("\\\\.\\HelloDDK"),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Open Device failed!\n");
		return 1;
	}

	HANDLE hThread[2];
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

	//创建IRP_MJ_CLEANUP IRP
	CloseHandle(hDevice);

    return 0;
}


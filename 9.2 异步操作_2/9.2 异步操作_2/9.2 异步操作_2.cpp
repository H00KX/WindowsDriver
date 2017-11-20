// 9.2 异步操作_2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>

#define DEVICE_NAME _T("test.dat")
#define BUFFER_SIZE	512

VOID CALLBACK MyFileIOCompletionRoutine(
	DWORD dwErrorCode,
	DWORD dwNumberOfBytes,
	LPOVERLAPPED lpOverlapped
)
{
	printf("Io operation end!\n");
}

int main()
{
	HANDLE hDevice = CreateFile(
		DEVICE_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Read Error\n");
		return 1;
	}

	UCHAR buffer[BUFFER_SIZE];

	OVERLAPPED overlap = { 0 };

	ReadFileEx(hDevice, buffer, BUFFER_SIZE, &overlap, MyFileIOCompletionRoutine);

	SleepEx(0, TRUE);

	CloseHandle(hDevice);

    return 0;
}


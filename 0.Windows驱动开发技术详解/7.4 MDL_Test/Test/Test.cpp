// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>

#define GetFilePointer(hFile) SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

int main()
{
	HANDLE hDevice = CreateFile("\\\\.\\HelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyWDMDevice", GetLastError());
		return 1;
	}

	UCHAR buffer[10];
	memset(buffer, 0xBB, 10);
	ULONG ulRead;
	BOOL bRet;
	bRet = ReadFile(hDevice, buffer, 10, &ulRead, NULL);
	if (bRet)
	{
		printf("Address 0X%08X\n", buffer);
		printf("(Read %d bytes):", ulRead);
		for (int i = 0; i<(int)ulRead; i++)
		{
			printf("%02X ", buffer[i]);
		}

		printf("\n");
	}

	CloseHandle(hDevice);

    return 0;
}


// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>


int main()
{
	HANDLE hDevice = CreateFile("\\\\.\\HelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyNTDevice", GetLastError());
		return 1;
	}

	UCHAR buffer[10];
	ULONG ulRead;
	BOOL bRet = ReadFile(hDevice, buffer, 10, &ulRead, NULL);
	if (bRet)
	{
		printf("Read %d bytes:", ulRead);
		for (int i = 0; i < (int)ulRead; ++i)
		{
			printf("%02X ", buffer[i]);
		}
		printf("\n");
	}

	CloseHandle(hDevice);

    return 0;
}


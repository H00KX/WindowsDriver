// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <winioctl.h>
#include "Ioctls.h"

int main()
{
	HANDLE hDevice =
		CreateFile("\\\\.\\HelloDDK",
			GENERIC_READ | GENERIC_WRITE,
			0,		// share mode none
			NULL,	// no security
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);		// no template

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyWDMDevice", GetLastError());
		return 1;
	}

	UCHAR InputBuffer[10];
	UCHAR OutputBuffer[10];

	memset(InputBuffer, 0xBB, 10);
	DWORD dwOutput;

	BOOL bRet;
	bRet = DeviceIoControl(hDevice, IOCTL_TEST1, InputBuffer, 10, OutputBuffer, 10, &dwOutput, NULL);
	if (bRet)
	{
		printf("Output buffer:%d bytes\n", dwOutput);
		for (int i = 0; i < (int)dwOutput; ++i)
		{
			printf("%02X ", OutputBuffer[i]);
		}
		printf("\n");
	}

	bRet = DeviceIoControl(hDevice, IOCTL_TEST2, InputBuffer, 10, &OutputBuffer, 10, &dwOutput, NULL);
	if (bRet)
	{
		printf("Output buffer:%d bytes\n", dwOutput);
		for (int i = 0; i<(int)dwOutput; i++)
		{
			printf("%02X ", OutputBuffer[i]);
		}
		printf("\n");
	}

	bRet = DeviceIoControl(hDevice, IOCTL_TEST3, InputBuffer, 10, &OutputBuffer, 10, &dwOutput, NULL);
	if (bRet)
	{
		printf("Output buffer:%d bytes\n", dwOutput);
		for (int i = 0; i<(int)dwOutput; i++)
		{
			printf("%02X ", OutputBuffer[i]);
		}
		printf("\n");
	}

	CloseHandle(hDevice);

    return 0;
}


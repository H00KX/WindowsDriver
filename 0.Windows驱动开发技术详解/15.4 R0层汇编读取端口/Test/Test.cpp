// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>

#include <winioctl.h>
#include "ioctls.h"


int main()
{
	HANDLE hDevice = CreateFile(_T("\\\\.\\HelloDDK"),
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

	DWORD dwOutput;
	DWORD inputBuffer[3] =
	{
		0x378,//对0x378进行操作
		1,//1代表8位操作，2代表16位操作，4代表32位操作
		1//输出字节0
	};

	char dwOutputBuff = 0;
	DWORD dwOutputLen;
	//类似于Out_8((PUCHAR)0x378,0);
	DeviceIoControl(hDevice, WRITE_PORT, inputBuffer, sizeof(inputBuffer), &dwOutputBuff, sizeof(dwOutputBuff), &dwOutput, NULL);
	printf("Write: %d\n", dwOutputBuff);
	DeviceIoControl(hDevice, READ_PORT, inputBuffer, sizeof(inputBuffer), &dwOutputBuff, sizeof(dwOutputBuff), &dwOutput, NULL);

	printf("Write: %d\n", dwOutputBuff);


	CloseHandle(hDevice);

    return 0;
}


// Test.cpp : �������̨Ӧ�ó������ڵ㡣
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
		0x378,//��0x378���в���
		1,//1����8λ������2����16λ������4����32λ����
		1//����ֽ�0
	};

	char dwOutputBuff = 0;
	DWORD dwOutputLen;
	//������Out_8((PUCHAR)0x378,0);
	DeviceIoControl(hDevice, WRITE_PORT, inputBuffer, sizeof(inputBuffer), &dwOutputBuff, sizeof(dwOutputBuff), &dwOutput, NULL);
	printf("Write: %d\n", dwOutputBuff);
	DeviceIoControl(hDevice, READ_PORT, inputBuffer, sizeof(inputBuffer), &dwOutputBuff, sizeof(dwOutputBuff), &dwOutput, NULL);

	printf("Write: %d\n", dwOutputBuff);


	CloseHandle(hDevice);

    return 0;
}


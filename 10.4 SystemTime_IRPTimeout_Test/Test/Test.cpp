// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <winioctl.h>
#include "Ioctls.h"

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
			"MyNtDevice", GetLastError());
		return 1;
	}
	DWORD dwOutput;

	DeviceIoControl(hDevice, IOCTL_SYSTEM_TIME, NULL, 0, NULL, 0, &dwOutput, NULL);

	printf("====================Complete DeviceIoControl=============================\n");
	DWORD dwRead;
	ReadFile(hDevice, NULL, 0, &dwRead, NULL);

	ReadFile(hDevice, NULL, 0, &dwRead, NULL);


	CloseHandle(hDevice);

    return 0;
}


// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
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
			"MyDevice", GetLastError());
		return 1;
	}
	WCHAR* InputBuffer = L"\\Driver\\ACPI";
	DWORD dwOutput;

	BOOL bRet;
	bRet = DeviceIoControl(hDevice, IOCTL_DUMP_DEVICE_STACK, InputBuffer, wcslen(InputBuffer) * 2 + 2, NULL, 0, &dwOutput, NULL);

	CloseHandle(hDevice);

    return 0;
}


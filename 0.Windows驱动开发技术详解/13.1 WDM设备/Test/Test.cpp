// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <winioctl.h>

#include "function.h"
#include "guid.h"

int main()
{
	HANDLE hDevice = GetDeviceViaInterface((LPGUID)&MY_WDM_DEVICE, 0);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyWDMDevice", GetLastError());
		return 1;
	}

	CloseHandle(hDevice);


    return 0;
}


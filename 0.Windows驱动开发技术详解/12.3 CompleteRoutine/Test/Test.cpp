// Test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>

int main()
{
	HANDLE hDevice = CreateFile(_T("\\\\.\\HelloDDKA"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device "
			"with Win32 error code: %d\n",
			GetLastError());
		return 1;
	}

	DWORD dRet;
	ReadFile(hDevice, NULL, 0, &dRet, NULL);

	CloseHandle(hDevice);

    return 0;
}


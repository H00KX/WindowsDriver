// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>


int main()
{
	HANDLE hDevice = CreateFile("\\\\.\\MyHelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Open the %s failure, the error code is: %d\n", "HelloDDK", GetLastError());
		return -1;
	}

	DWORD dwLength;
	DWORD dwRed;
	dwRed = GetFileSize(hDevice, &dwLength);
	printf("The file read %d length is %d\n", dwRed,dwLength);

	char szWrite[MAX_PATH] = "Hello World";
	BOOL bRet = WriteFile(hDevice, szWrite, sizeof(szWrite), &dwLength, NULL);

	if (bRet)
	{
		printf("Write %d bytes: %s\n", dwLength, szWrite);
	}

	memset(szWrite, '\0', MAX_PATH);

	bRet = ReadFile(hDevice, szWrite, 5, &dwLength, NULL);
	if (bRet)
	{
		printf("Read %d bytes is: %s\n", dwLength, szWrite);
	}

	 dwRed = GetFileSize(hDevice, &dwLength);
	printf("The read %d file length: %d\n", dwRed,dwLength);


    return 0;
}


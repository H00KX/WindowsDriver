// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <winioctl.h>

#define LinkName	L"\\\\.\\HelloDev"
#define	EVENT_NAME	L"ProcWatch"

#define IOCTRL_BASE		0x800

#define MyCode(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define RECV_INFO	MyCode(0)
#define RECV_EVENT	MyCode(1)

typedef struct _Info {
	HANDLE hProcessId;
	BOOLEAN Create;
}INFO, *PINFO;

int main()
{
	HANDLE hDevice = CreateFile(LinkName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
	{
		printf("未打开设备\n");
		return 0;
	}

	/*HANDLE event = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_NAME);
	if (event == NULL)
	{
		CloseHandle(hDevice);
		printf("event open failure, code: %X\n", GetLastError());
		return 0;
	}*/
	DWORD dwRet;
	HANDLE event = CreateEvent(NULL,TRUE,FALSE,NULL);
	if (event == NULL)
	{
		CloseHandle(hDevice);
		printf("event create failure, code: %X\n", GetLastError());
		return 0;
	}

	DeviceIoControl(hDevice, RECV_EVENT, &event, sizeof(HANDLE), NULL, 0, &dwRet, NULL);

	

	int i = 3;
	while (1)
	{
		WaitForSingleObject(event, INFINITE);
		//ResetEvent(event);
		INFO info;
		
		DeviceIoControl(hDevice, RECV_INFO, NULL,0, &info, sizeof(INFO),&dwRet,NULL);

		if (info.Create)
		{
			printf("启动进程:%d\n", info.hProcessId);
		}
		else
		{
			printf("关闭进程:%d\n", info.hProcessId);
		}
		
		
	}

    return 0;
}


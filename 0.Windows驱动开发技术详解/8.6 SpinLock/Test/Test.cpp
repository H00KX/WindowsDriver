// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <winioctl.h>
#include "ioctls.h"

UINT WINAPI Thread1(LPVOID pContext)
{
	BOOL bRet;
	DWORD dwOutput;
	//发送IOCTL码
	bRet = DeviceIoControl(*(PHANDLE)pContext, IOCTL_TEST1, NULL, 0, NULL, 0, &dwOutput, NULL);

	return 0;
}


int main()
{
	//打开设备
	HANDLE hDevice = CreateFile("\\\\.\\HelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	//判断是否成功打开设备句柄
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyWDMDevice", GetLastError());
		return 1;
	}
	HANDLE hThread[2];
	//开启两个线程，每个线程都去执行DeviceIoControl
	//因此在IRP_MJ_DEVICE_CONTROL的派遣函数会并行运行
	//
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);

	//等待两个进程全部执行完毕
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

	//关闭句柄
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	CloseHandle(hDevice);

    return 0;
}


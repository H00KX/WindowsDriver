// Test.cpp : �������̨Ӧ�ó������ڵ㡣
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
	//����IOCTL��
	bRet = DeviceIoControl(*(PHANDLE)pContext, IOCTL_TEST1, NULL, 0, NULL, 0, &dwOutput, NULL);

	return 0;
}


int main()
{
	//���豸
	HANDLE hDevice = CreateFile("\\\\.\\HelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	//�ж��Ƿ�ɹ����豸���
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyWDMDevice", GetLastError());
		return 1;
	}
	HANDLE hThread[2];
	//���������̣߳�ÿ���̶߳�ȥִ��DeviceIoControl
	//�����IRP_MJ_DEVICE_CONTROL����ǲ�����Ტ������
	//
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hDevice, 0, NULL);

	//�ȴ���������ȫ��ִ�����
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

	//�رվ��
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	CloseHandle(hDevice);

    return 0;
}


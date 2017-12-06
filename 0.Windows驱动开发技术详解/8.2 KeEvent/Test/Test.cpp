// Test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <WinIoCtl.h>

#define IOCTL_TRANSMIT_EVENT CTL_CODE(FILE_DEVICE_UNKNOWN, \
	0x801, \
	METHOD_BUFFERED, \
	FILE_ANY_ACCESS)


unsigned _stdcall Thread1(void *pParam)
{

	return 0;
}

int main()
{
	HANDLE hDevice = CreateFile("\\\\.\\HelloDDK",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	//�ж��豸�Ƿ��
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n",
			"MyDDKDevice", GetLastError());
		return 1;
	}
	BOOL bRet;
	DWORD dwOutput;
	//�����û�ģʽͬ���¼�
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//���������߳�
	HANDLE hThread1 = (HANDLE)_beginthreadex(NULL, 0, Thread1, &hEvent, 0, NULL);
	//���û�ģʽ���¼�������ݸ�����
	bRet = DeviceIoControl(hDevice, 
		IOCTL_TRANSMIT_EVENT,
		&hEvent,
		sizeof(hEvent),
		NULL,
		0,
		&dwOutput,
		NULL);

	WaitForSingleObject(hThread1, INFINITE);

	//�رո������
	CloseHandle(hDevice);
	CloseHandle(hThread1);
	CloseHandle(hEvent);

    return 0;
}


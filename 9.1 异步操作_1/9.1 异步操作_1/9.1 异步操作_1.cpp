// 9.1 �첽����_1.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>

#define BUFFER_SIZE		512

int main()
{
	HANDLE hDevice = 
		CreateFile(_T("test.dat"),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		printf("Read Error\n");
		return 1;
	}

	UCHAR buffer[BUFFER_SIZE];
	DWORD dwRead;

	//��ʼ��overlapʹ��ȫ��Ϊ0
	OVERLAPPED overlap = { 0 };

	//����overlap�¼�
	overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ReadFile(hDevice, buffer, BUFFER_SIZE, &dwRead, &overlap);

	Sleep(5000);

	WaitForSingleObject(overlap.hEvent, INFINITE);
	CloseHandle(hDevice);

    return 0;
}


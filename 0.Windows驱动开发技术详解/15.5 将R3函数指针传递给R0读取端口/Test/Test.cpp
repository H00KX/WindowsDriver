// Test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
//使用CTL_CODE必须加入winioctl.h
#include <winioctl.h>
#include "ioctls.h"

UCHAR In_8(PUCHAR Port)
{
	UCHAR Value;
	__asm
	{
		mov edx, Port
		in al, dx
		mov Value, al
		//插入几个空指令
		nop
		nop
	}

	return(Value);
}

USHORT In_16(PUSHORT Port)
{
	USHORT Value;

	__asm
	{
		mov edx, Port
		in ax, dx
		mov Value, ax
		//插入几个空指令
		nop
		nop
	}
	return(Value);
}

ULONG In_32(PULONG Port)
{
	ULONG Value;
	__asm
	{
		mov edx, Port
		in eax, dx
		mov Value, eax
		//插入几个空指令
		nop
		nop
	}
	return(Value);
}

void Out_32(PULONG Port, ULONG Value)
{
	__asm
	{
		mov edx, Port
		mov eax, Value
		out dx, eax
		//插入几个空指令
		nop
		nop
	}
}

void Out_16(PUSHORT Port, USHORT Value)
{
	__asm
	{
		mov edx, Port
		mov ax, Value
		out dx, ax
		//插入几个空指令
		nop
		nop
	}
}

void Out_8(PUCHAR Port, UCHAR Value)
{
	__asm
	{
		mov edx, Port
		mov al, Value
		out dx, al
		//插入几个空指令
		nop
		nop
	}
}

void KernelModeFunction()
{
	OutputDebugString(_T("KernelModeFunction"));
	Out_8((PUCHAR)0x378, 0);
}

int main()
{
	HANDLE hDevice = CreateFile(
		_T("\\\\.\\HelloDDK"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to device: "
			"%s with Win32 error code: %d\n"
			"MyWDMDevice", GetLastError());
		return 1;
	}
	DWORD dwOutput;

	DWORD Function_Address = (DWORD)KernelModeFunction;
	DeviceIoControl(hDevice, IOCTL_KERNEL_FUNCTION, &Function_Address, 4, NULL, 0, &dwOutput, NULL);

	CloseHandle(hDevice);

	return 0;
}




// 15.1 只接R3汇编读取端口信息.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>

UCHAR In_8(PUCHAR Port)
{
	UCHAR Value;
	__asm
	{
		mov edx, Port
		in al,dx
		mov Value, al
		nop
		nop
	}
	return Value;
}

USHORT In_16(PUSHORT Port)
{
	USHORT Value;

	__asm
	{
		mov edx, Port
		in ax, dx
		mov Value, ax
		nop
		nop
	}
	return Value;
}

ULONG In_32(PULONG Port)
{
	ULONG Value;
	__asm
	{
		mov edx, Port;
		in eax, dx;
		mov Value, eax;
		nop;
		nop;
	}
	return Value;
}

void Out_32(PULONG Port, ULONG Value)
{
	__asm
	{
		mov edx, Port;
		mov eax, Value;
		out dx, eax;
		nop;
		nop;
	}
}

void Out_16(PUSHORT Port, USHORT Value)
{
	__asm
	{
		mov edx, Port;
		mov ax, Value;
		out dx, ax;
		nop;
		nop;
	}
}

void Out_8(PUCHAR Port, UCHAR Value)
{
	__asm
	{
		mov edx, Port;
		mov al, Value;
		out dx, al;
		nop;
		nop;
	}
}


int main()
{
	Out_8((PUCHAR)0x378, 0);

    return 0;
}


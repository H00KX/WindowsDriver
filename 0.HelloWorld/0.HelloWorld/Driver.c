#include <ntddk.h>

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint(("good bye\n"));
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING  ustrRegistPath)
{
	DbgPrint(("first: Hello, my salary!\n"));

	// 设置一个卸载函数便于这个函数能退出。  
	pDriverObject->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}



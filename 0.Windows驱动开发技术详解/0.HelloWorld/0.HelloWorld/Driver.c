#include <ntddk.h>

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint(("good bye\n"));
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING  ustrRegistPath)
{
	DbgPrint(("first: Hello, my salary!\n"));

	// ����һ��ж�غ�����������������˳���  
	pDriverObject->DriverUnload = DriverUnload;

	return STATUS_SUCCESS;
}



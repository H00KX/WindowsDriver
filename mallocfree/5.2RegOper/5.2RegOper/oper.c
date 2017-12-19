#include <ntddk.h>
#include <wchar.h>


#define		DeviceName		L"\\Device\\HelloDDK"
#define		LinkName		L"\\??\\HelloDDK"

#define		RegPath			L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control"
#define		NewRegPath		L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\RegNew"

VOID CreateMulKey(VOID)
{
	HANDLE hReg = NULL;
	NTSTATUS status;
	OBJECT_ATTRIBUTES	objAttrib;
	UNICODE_STRING uReg;
	WCHAR buff[1024] = { 0 };
	ULONG leng = 0;
	ULONG creAttr = 0;
	UNICODE_STRING uRegValueKey;

	RtlInitUnicodeString(&uReg, NewRegPath);

	InitializeObjectAttributes(
		&objAttrib,
		&uReg,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	creAttr = REG_CREATED_NEW_KEY;
	status = ZwCreateKey(
		&hReg,
		KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
		&objAttrib,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		&creAttr
	);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwCreateKey is Failure, status: %x\n", status));
		return;
	}

	
	swprintf(buff, L"%ws%wc%ws%wc%wc", L"hello world", 0,L"the next string",0, 0);
	leng = (wcslen(L"hello worldthe next string") + 3) * sizeof(WCHAR);

	RtlInitUnicodeString(&uRegValueKey, L"Hello");

	status = ZwSetValueKey(hReg,
		&uRegValueKey,
		0,
		REG_MULTI_SZ,
		buff,
		leng);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwSetValueKey Ê§°Ü £¬ status: %x\n", status));
		return;
	}

	

	KdPrint(("ÉèÖÃ³É¹¦\n"));
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{

}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	pDriverObject->DriverUnload = DriverUnload;

	CreateMulKey();

	return STATUS_SUCCESS;
}

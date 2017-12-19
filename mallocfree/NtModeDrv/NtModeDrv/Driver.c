#include <ntddk.h>

#define DEVICE_NAME		L"\\Driver\\NtModeDrv"
#define LINK_NAME		L"\\??\\NtModeDrv"

#define IOCTRL_BASE		0x800

#define MYIOCTRL_CODE(i)	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PRINT_CODE		MYIOCTRL_CODE(0)
#define BYE_CODE		MYIOCTRL_CODE(1)
#define WRITE_HELLO		MYIOCTRL_CODE(2)

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	PVOID pBuff = pIrp->AssociatedIrp.SystemBuffer;
	ULONG uReadLen = stack->Parameters.Read.Length;

	wchar_t buff[] = L"Hello World";
	
	ULONG uLen = uReadLen < wcslen(buff) * sizeof(wchar_t) ? uReadLen : wcslen(buff) * sizeof(wchar_t);

	memcpy(pBuff, buff, uLen);

	pIrp->IoStatus.Information = uLen;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	return STATUS_SUCCESS;
}

NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);


	ULONG InputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

	PVOID pInput = pIrp->AssociatedIrp.SystemBuffer;
	PVOID pOutput = pIrp->AssociatedIrp.SystemBuffer;

	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;


	switch (code)
	{
	case PRINT_CODE:
	{
		KdPrint(("%s",pInput));
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
	}
		break;

	case BYE_CODE:
	{
		KdPrint(("Bye\n"));
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
	}
		break;

	case WRITE_HELLO:
	{
		wchar_t buff[] = L"Hello World";

		ULONG uLen = OutputLen < wcslen(buff) * sizeof(wchar_t) ? OutputLen : wcslen(buff) * sizeof(wchar_t);

		memcpy(pOutput, buff, uLen);

		pIrp->IoStatus.Information = uLen;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
	}
		break;

	default:
		break;
	}
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING ustrDevName;
	UNICODE_STRING ustrLinkName;
	NTSTATUS status;

	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);

	status = IoCreateDevice(pDriverObject,
		0,
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	pDevObj->Flags |= DO_BUFFERED_IO;

	RtlInitUnicodeString(&ustrLinkName, LINK_NAME);

	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}

	return status;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING uLinkName = { 0 };
	RtlInitUnicodeString(&uLinkName, LINK_NAME);

	IoDeleteSymbolicLink(&uLinkName);

	IoDeleteDevice(pDriverObject->DeviceObject);

	KdPrint(("Driver Unload\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegString)
{
	
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;

	pDriverObject->DriverUnload = DriverUnload;

	return CreateDevice(pDriverObject);
}
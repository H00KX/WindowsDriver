#include "Driver.h"

#define		BUFFER_SIZE		1024
VOID RtlTest()
{
	PUCHAR pBuffer = (PUCHAR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	//ÓÃ0Ìî³äÄÚ´æ
	RtlZeroMemory(pBuffer, BUFFER_SIZE);

	PUCHAR pBuffer2 = (PUCHAR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	//ÓÃ¹Ì¶¨×Ö½ÚÌî³äÄÚ´æ
	RtlFillMemory(pBuffer2, BUFFER_SIZE, 0xAA);

	//ÄÚ´æ¿½±´
	RtlCopyMemory(pBuffer, pBuffer2, BUFFER_SIZE);

	//ÅÐ¶ÏÄÚ´æÊÇ·ñÒ»ÖÂ
	ULONG ulRet = RtlCompareMemory(pBuffer, pBuffer2, BUFFER_SIZE);
	if (ulRet == BUFFER_SIZE)
	{
		KdPrint(("The two blocks are same.\n"));
	}
}



extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));
	
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	status = CreateDevice(pDriverObject);

	RtlTest();

	KdPrint(("DriverEntry end\n"));
	return status;
}


NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING ustrDevName;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\MyDDKDriver");
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	UNICODE_STRING ustrSymLinkName;
	RtlInitUnicodeString(&ustrSymLinkName, L"\\??\\HelloDDKDriver");
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = ustrDevName;
	pDevExt->ustrSymLinkName = ustrSymLinkName;

	status = IoCreateSymbolicLink(&ustrSymLinkName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return status;

}

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING ustrDevName;

	KdPrint(("Enter Unload\n"));

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\MyDDKDriver");

	UNICODE_STRING ustrSymLinkName;
	RtlInitUnicodeString(&ustrSymLinkName, L"\\??\\HelloDDKDriver");

	IoDeleteSymbolicLink(&ustrSymLinkName);

	PDEVICE_OBJECT pDevObj = pDriverObject->DeviceObject;
	IoDeleteDevice(pDevObj);
	
	KdPrint(("Leave Unload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevOjb, PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}


#include "DriverA.h"
#define DeviceName	L"\\Device\\MyDDKDeviceA"
#define LinkName	L"\\??\\HelloDDKA"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("DriverA:Enter A DriverEntry!\n"));

	//
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->DriverUnload = HelloDDKUnload;

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	KdPrint(("DriverA: Leave A DriverEntry"));
	return status;
}

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

	//创建设备
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,TRUE,
		&pDevObj);

	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_DIRECT_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, LinkName);
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}



VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextObj;
	KdPrint(("DriverA: Enter A DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("DriverA: Leave A DriverUnload\n"));
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverA: Enter A HelloDDKRead\n"));
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	ULONG totalLength;
	PVOID virtualAddress;
	if (!pIrp->MdlAddress)
	{
		status = STATUS_UNSUCCESSFUL;
		totalLength = 0;
		goto HelloDDKRead_EXIT;
	}
	virtualAddress = MmGetMdlVirtualAddress(pIrp->MdlAddress);
	totalLength = MmGetMdlByteCount(pIrp->MdlAddress);

	RtlFillMemory(virtualAddress, totalLength, 0xFF);

	KdPrint(("DriverA:virtualAddress: %x\n", virtualAddress));
	KdPrint(("DriverA: totalLength: %d\n", totalLength));

HelloDDKRead_EXIT:
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = totalLength;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverA: Leave A HelloDDKRead\n"));

	return status;
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverA:Enter A HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverA: Leave A HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverA:Enter A HelloDDKCreate\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverA:Leave A HelloDDKCreate\n"));
	return status;
}

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverA:Enter A HelloDDKClose\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverA:Leave A HelloDDKClose\n"));
	return status;
}

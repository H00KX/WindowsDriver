#pragma once
#include "DriverB.h"

#define DeviceNameA L"\\Device\\MyDDKDeviceA"
#define LinkNameA	L"\\??\\HelloDDKDeviceA"

#define DeviceNameB	L"\\Device\\MyDDKDeviceB"
#define LinkNameB	L"\\??\\HelloDDKDeviceB"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS ntStatus;
	KdPrint(("DriverB: Enter B DriverEntry\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	UNICODE_STRING DeviceName;
	RtlInitUnicodeString(&DeviceName, DeviceNameA);

	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT FileObject = NULL;
	ntStatus = IoGetDeviceObjectPointer(&DeviceName, FILE_ALL_ACCESS, &FileObject, &DeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("DriverB: IoGetDeviceObjectPointer() 0x%x\n", ntStatus));
		return ntStatus;
	}

	ntStatus = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(ntStatus))
	{
		ObDereferenceObject(FileObject);
		KdPrint(("IoCreateDevice() 0x%x\n", ntStatus));
		return ntStatus;
	}

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	PDEVICE_OBJECT FilterDeviceObject = pdx->pDevice;

	PDEVICE_OBJECT TargetDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, DeviceObject);

	pdx->TargetDevice = TargetDevice;

	if (!TargetDevice)
	{
		ObDereferenceObject(FileObject);
		IoDeleteDevice(FilterDeviceObject);
		KdPrint(("IoAttachDeviceToDeviceStack() 0x%x\n", ntStatus));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FilterDeviceObject->DeviceType = TargetDevice->DeviceType;
	FilterDeviceObject->Characteristics = TargetDevice->Characteristics;
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	FilterDeviceObject->Flags |= (TargetDevice->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO));

	ObDereferenceObject(FileObject);

	KdPrint(("DriverB: B attached A successfully!\n"));

	KdPrint(("DriverB: Leave B DriverEntry!\n"));

	return ntStatus;
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ntStatus;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceNameB);

	//创建设备
	ntStatus = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;
	return STATUS_SUCCESS;
}

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("DriverB:Enter B DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;

	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;
		pNextObj = pNextObj->NextDevice;
		//从设备栈中弹出
		IoDetachDevice(pDevExt->TargetDevice);
		//删除该设备对象
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("DriverB:Enter B DriverUnload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("DriverB:Enter B HelloDDKDispatchRoutine\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverB:Leave B HelloDDKDispatchRoutine\n"));
	return ntStatus;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("DriverB:Enter B HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB:Leave B HelloDDKCreate\n"));

	return ntStatus;
}

NTSTATUS
MyIoCompletion(IN PDEVICE_OBJECT  DeviceObject,IN PIRP  Irp,IN PVOID  Context)
{
	if (Irp->PendingReturned == TRUE)
	{
		//设置事件
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKRead\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoCopyCurrentIrpStackLocationToNext(pIrp);

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	//设置完成例程
	IoSetComple
}

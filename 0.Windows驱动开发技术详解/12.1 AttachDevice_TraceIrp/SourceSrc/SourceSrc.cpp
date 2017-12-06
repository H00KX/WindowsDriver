#include "SourceSrv.h"

#define TargetDeviceName L"\\Device\\TargetDevice"
#define TargetLinkName	L"\\??\\MyTargetDevice"

#define DeviceName	L"\\Device\\SourceDevice"
#define LinkName	L"\\??\\MySourceDevice"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS ntStatus;
	KdPrint(("Source: Enter source DriverEntry\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	UNICODE_STRING ustrTargetDevName;
	RtlInitUnicodeString(&ustrTargetDevName, TargetDeviceName);

	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT FileObject = NULL;
	ntStatus = IoGetDeviceObjectPointer(&ustrTargetDevName, FILE_ALL_ACCESS, &FileObject, &DeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("Source:IoGetDeviceObjectPointer() 0x%x\n", ntStatus));
		return ntStatus;
	}


	ntStatus = CreateDevice(pDriverObject);

	if (!NT_SUCCESS(ntStatus))
	{
		ObDereferenceObject(FileObject);
		DbgPrint("IoCreateDevice() 0x%x!\n", ntStatus);
		return ntStatus;
	}

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	PDEVICE_OBJECT FilterDeviceObject = pdx->pDevice;

	PDEVICE_OBJECT TargetDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, DeviceObject);

	//将底层设备对象记录下来
	pdx->TargetDevice = TargetDevice;

	if (!TargetDevice)
	{
		ObDereferenceObject(FileObject);
		IoDeleteDevice(FilterDeviceObject);
		KdPrint(("IoAttachDeviceToDeviceStack() 0x%x!\n", ntStatus));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FilterDeviceObject->DeviceType = TargetDevice->DeviceType;
	FilterDeviceObject->Characteristics = TargetDevice->Characteristics;
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	FilterDeviceObject->Flags |= (TargetDevice->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO));

	ObDereferenceObject(FileObject);

	KdPrint(("Source: Source Attached Successfully!\n"));

	KdPrint(("Source: Leave Target DriverEntry!\n"));

}


NTSTATUS CreateDevice(IN PDRIVER_OBJECT	pDriverObject)
{
	NTSTATUS ntStatus;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

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
	KdPrint(("Source:Enter Target DriverUnload\n"));
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
	KdPrint(("Source:Enter Target DriverUnload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Source:Enter source HelloDDKDispatchRoutine\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Source:Leave source HelloDDKDispatchRoutine\n"));
	return ntStatus;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("Source:Enter Source HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//
	// 	// 完成IRP
	// 	pIrp->IoStatus.Status = ntStatus;
	// 	pIrp->IoStatus.Information = 0;	// bytes xfered
	// 	IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("Source:Leave Source HelloDDKCreate\n"));

	return ntStatus;
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("Source: Enter Source HelloDDKRead\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	//调用底层驱动
	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("Source: Leave Source HelloDDKRead\n"));

	return ntStatus;
}


NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("Source:Enter Source HelloDDKClose\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("Source:Leave Source HelloDDKClose\n"));

	return ntStatus;
}

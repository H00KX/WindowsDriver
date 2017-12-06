#include "Source.h"

#define DeviceName		L"\\Device\\SourceDevice"
#define LinkName		L"\\??\\MySourceDevice"

#define TargetDeviceName	L"\\Device\\TargetDevice"
#define TargetLinkName	L"\\??\\MyTargetDevice"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS ntStatus;
	KdPrint(("Source: Enter Source DriverEntry\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	ntStatus = CreateDevice(pDriverObject);

	KdPrint(("Source: Leave Source DriverEntry\n"));

	return ntStatus;
}

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ntStatus;
	PDEVICE_OBJECT	pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

	//创建设备
	ntStatus = IoCreateDevice(
		pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj
	);

	if (!NT_SUCCESS(ntStatus))
	{
		return ntStatus;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, LinkName);
	pDevExt->ustrSymLinkName = symLinkName;
	NTSTATUS status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Source:Enter Source DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;

	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;

		//删除符号链接
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("Source:Enter Source DriverUnload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Target:Enter Target HelloDDKDispatchRoutine\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKDispatchRoutine\n"));
	return ntStatus;
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Source: Enter Source HelloDDKRead\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	UNICODE_STRING DevName;
	RtlInitUnicodeString(&DevName, TargetDeviceName);

	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT FileObject = NULL;
	ntStatus = IoGetDeviceObjectPointer(&DevName, FILE_ALL_ACCESS, &FileObject, &DeviceObject);

	KdPrint(("Source: FileObject: %x\n", FileObject));
	KdPrint(("Source: DeviceObject: %x\n", DeviceObject));

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("Source: IoGetDeviceObjectPointer() 0x%x\n", ntStatus));

		ntStatus = STATUS_UNSUCCESSFUL;

		pIrp->IoStatus.Status = ntStatus;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		return ntStatus;
	}

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IO_STATUS_BLOCK status_block;
	LARGE_INTEGER offset = RtlConvertLongToLargeInteger(0);

	PIRP pNewIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
		DeviceObject,
		NULL,
		0, &offset,
		&status_block);
	//
	KdPrint(("pNewIrp->UserEvent: %x\n", pNewIrp->UserEvent));
	pNewIrp->UserEvent = &event;

	KdPrint(("Source: pNewIrp: %x\n", pNewIrp));

	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(pNewIrp);
	stack->FileObject = FileObject;

	NTSTATUS status = IoCallDriver(DeviceObject, pNewIrp);

	if (status == STATUS_PENDING)
	{
		status = KeWaitForSingleObject(&event,
			Executive,
			KernelMode,
			FALSE,
			NULL);
		status = status_block.Status;
	}
	
	ObDereferenceObject(FileObject);

	ntStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Source: Leave Source HelloDDKRead\n"));

	return ntStatus;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Source:Enter Source HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Source:Leave Source HelloDDKCreate\n"));

	return ntStatus;
}

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Source:Enter Source HelloDDKClose\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Source:Leave Source HelloDDKClose\n"));

	return ntStatus;
}

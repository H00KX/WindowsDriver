#include "DriverB.h"

#define DeviceNameA		L"\\Device\\MyDDKDeviceA"
#define LinkNameA		L""

#define DeviceNameB		L"\\Device\\MyDDKDeviceB"
#define LinkNameB		L""


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
		KdPrint(("DeviceB: IoGetDeviceObjectPointer() 0x%x\n", ntStatus));
		return ntStatus;
	}

	//创建自己的驱动B
	ntStatus = CreateDevice(pDriverObject);

	if (!NT_SUCCESS(ntStatus))
	{
		ObDereferenceObject(FileObject);
		KdPrint(("IoCreateDevice() 0x%x\n", ntStatus));
		return ntStatus;
	}

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	PDEVICE_OBJECT FilterDeviceObject = pdx->pDevice;

	//将自己的设备对象挂载在 DriverA 设备对象上
	PDEVICE_OBJECT TargetDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, DeviceObject);

	//将底层设备记录下来
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

	KdPrint(("DriverB: B attached A successfully!\n"));
	KdPrint(("DriverB: Leave B DriverEntry!\n"));

	return ntStatus;
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ntStatus;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceNameB);

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

VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextObj;
	KdPrint(("DriverB: Enter B DriverUnlaod\n"));
	pNextObj = pDriverObject->DeviceObject;

	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
		pNextObj = pNextObj->NextDevice;
		//弹出附加设备
		IoDetachDevice(pDevExt->TargetDevice);
		//删除设备
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("DeviceB: Enter B DriverUnlaod\n"));
}
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKRoutine\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB: Leave B HelloDDKRoutine\n"));

	return ntStatus;
}

NTSTATUS HelloDDKCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB: Leave B HelloDDKCreate\n"));

	return ntStatus;
}

NTSTATUS MyIoCompletion(PDEVICE_OBJECT DeviceObject, PIRP pIrp, PVOID Context)
{
	KdPrint(("Enter MyIoCompletion\n"));
	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}
	return STATUS_SUCCESS;
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKRead\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	//将当前IRP堆栈拷贝给底层驱动
	IoCopyCurrentIrpStackLocationToNext(pIrp);

	//设置完成例程
	IoSetCompletionRoutine(pIrp, MyIoCompletion, NULL, TRUE, TRUE, TRUE);

	//调用底层驱动
	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	if (ntStatus == STATUS_PENDING)
	{
		KdPrint(("STATUS_PENDING\n"));
	}

	ntStatus = STATUS_PENDING;

	KdPrint(("DriverB: Leave B HelloDDKRead\n"));

	return ntStatus;
}

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKClose\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB: Leave B HelloDDKClose\n"));

	return ntStatus;
}

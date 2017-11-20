#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"


extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//注册其他驱动调用函数入口
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutin;
	}
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = HelloDDKCleanUp;


	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	KdPrint(("DriverEntry end\n"));
	return status;
}


NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT pDriverObject
)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;

	RtlInitUnicodeString(&devName, DevName);

	//创建设备
	status = IoCreateDevice(
		pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;
	pDevExt->pIRPLinkListHead = (PLIST_ENTRY)ExAllocatePool(PagedPool, sizeof(LIST_ENTRY));

	pDevExt->pIRPLinkListHead->Blink = pDevExt->pIRPLinkListHead;
	pDevExt->pIRPLinkListHead->Flink = pDevExt->pIRPLinkListHead;

	//创建符号链接
	UNICODE_STRING symLinkName;

	RtlInitUnicodeString(&symLinkName, SymLinkName);
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
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)                         //遍历设备对象，并删除
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//删除符号链接
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
}


NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	PMY_IRP_ENTRY pIrp_entry = (PMY_IRP_ENTRY)ExAllocatePool(PagedPool, sizeof(MY_IRP_ENTRY));

	pIrp_entry->pIRP = pIrp;

	LIST_ENTRY listHead;

	//插入队列
	InsertHeadList(pDevExt->pIRPLinkListHead, &pIrp_entry->ListEntry);

	//将IRP设置为挂起
	IoMarkIrpPending(pIrp);

	KdPrint(("Leave HelloDDKRead\n"));

	return STATUS_PENDING;
}

NTSTATUS HelloDDKCleanUp(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKCleanUp\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	//1. 将存在队列中的IRP逐个出队列，并处理
	PMY_IRP_ENTRY my_irp_entry;
	while (!IsListEmpty(pDevExt->pIRPLinkListHead))
	{
		PLIST_ENTRY pEntry = RemoveHeadList(pDevExt->pIRPLinkListHead);
		my_irp_entry = CONTAINING_RECORD(pEntry,
			MY_IRP_ENTRY,
			ListEntry);

		my_irp_entry->pIRP->IoStatus.Status = STATUS_SUCCESS;
		my_irp_entry->pIRP->IoStatus.Information = 0;
		IoCompleteRequest(my_irp_entry->pIRP, IO_NO_INCREMENT);

		ExFreePool(my_irp_entry);
	}
	ExFreePool(pDevExt->pIRPLinkListHead);
	//2. 处理IRP_MJ_CLEANUP的IRP
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);


	return STATUS_SUCCESS;
}

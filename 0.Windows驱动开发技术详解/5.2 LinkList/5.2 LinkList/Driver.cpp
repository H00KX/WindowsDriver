#include "Driver.h"

typedef struct _MYDATASTRUCT
{
	ULONG number;
	LIST_ENTRY ListEntry;
} MYDATASTRUCT, *PMYDATASTRUCT;

VOID LinkListTest()
{
	LIST_ENTRY linkListHead;
	//初始化链表
	InitializeListHead(&linkListHead);

	PMYDATASTRUCT pData;
	ULONG i = 0;
	//在链表中插入10个元素
	KdPrint(("Begin insert to link list"));
	for (i = 0; i < 10; ++i)
	{
		pData = (PMYDATASTRUCT)ExAllocatePool(PagedPool, sizeof(MYDATASTRUCT));
		pData->number = i;
		InsertTailList(&linkListHead, &pData->ListEntry);
	}

	//从链表中取出，并显示
	KdPrint(("Begin remove from link list\n"));
	while (!IsListEmpty(&linkListHead))
	{
		PLIST_ENTRY pEntry = RemoveTailList(&linkListHead);
		pData = CONTAINING_RECORD(pEntry, MYDATASTRUCT, ListEntry);
		KdPrint(("%d\n", pData->number));
		ExFreePool(pData);
	}

}


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//注册其他驱动调用函数入口
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	status = CreateDevice(pDriverObject);

	LinkListTest();

	KdPrint(("Leave DriverEntry\n"));

	return STATUS_SUCCESS;
}


NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{

	KdPrint(("Enter CreateDevice\n"));

	NTSTATUS status;
	UNICODE_STRING ustrDevName;

	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	RtlInitUnicodeString(&ustrDevName, L"\\Device\\MyDDKDevice");
	

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
	pDevObj->Flags |= DO_BUFFERED_IO;

	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDevName = ustrDevName;

	UNICODE_STRING ustrSymLinkName;
	RtlInitUnicodeString(&ustrSymLinkName, L"\\??\\HelloDDK");
	pDevExt->ustrSymLinkName = ustrSymLinkName;
	status = IoCreateSymbolicLink(&ustrSymLinkName, &ustrDevName);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
		
	}

	
	KdPrint(("Leave CreateDevice\n"));
	return status;;
}

VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Endter Unload\n"));

	PDEVICE_OBJECT pDevObj = NULL;

	pDevObj = pDriverObject->DeviceObject;
	while (pDevObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		UNICODE_STRING symLinkName;
		RtlInitUnicodeString(&symLinkName, L"\\??\\MyDDKDriver");
		IoDeleteSymbolicLink(&symLinkName);

		pDevObj = pDevObj->NextDevice;

		IoDeleteDevice(pDevExt->pDevice);

		/*
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		IoDeleteSymbolicLink(&pDevExt->ustrSymLinkName);
		pDevObj = pDevObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
		
		*/
	}
	KdPrint(("Leave Unload\n"));
}


NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter DispatchRoutine"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDispatchRoutine\n"));

	return STATUS_SUCCESS;
}



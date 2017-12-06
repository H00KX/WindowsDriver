#include "Driver.h"

typedef struct _MYDATASTRUCT
{
	CHAR buffer[64];
}MYDATASTRUCT, *PMYDATASTRUCT;

VOID LookasideTest()
{
	//初始化Lookaside
	PAGED_LOOKASIDE_LIST pageList;
	ExInitializePagedLookasideList(&pageList, NULL, NULL, 0, sizeof(MYDATASTRUCT), '1234', 0);

#define ARRAY_NUMBER	50
	PMYDATASTRUCT MyObjectArray[ARRAY_NUMBER];

	//模拟频发申请内存
	for (int i = 0; i < ARRAY_NUMBER; ++i)
	{
		MyObjectArray[i] = (PMYDATASTRUCT)ExAllocateFromPagedLookasideList(&pageList);
	}

	//模拟频繁回收内存
	for (int i = 0; i < ARRAY_NUMBER; ++i)
	{
		ExFreeToPagedLookasideList(&pageList, MyObjectArray[i]);
		MyObjectArray[i] = NULL;
	}

	ExDeletePagedLookasideList(&pageList);
	//删除Lookaside对象
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//注册其他驱动调用函数
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	LookasideTest();

	KdPrint(("DriverEntry end\n"));

	return status;
}


NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

	//创建设备
	status = IoCreateDevice(pDriverObject,
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
	
	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK");
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return status;
}

VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Enter Unload"));
	PDEVICE_OBJECT pDevObj = pDriverObject->DeviceObject;
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	UNICODE_STRING ustrSymLinkName;
	RtlInitUnicodeString(&ustrSymLinkName, L"\\??\\HelloDDK");
	IoDeleteSymbolicLink(&ustrSymLinkName);
	IoDeleteDevice(pDevObj);

	KdPrint(("Leave Unload"));
}

NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}


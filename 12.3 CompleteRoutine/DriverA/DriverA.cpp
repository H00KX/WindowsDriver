#include "DriverA.h"
#define DeviceName	L"\\Device\\MyDDKDeviceA"
#define LinkName	L"\\??\\HelloDDKA"


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("DriverA: Enter A DriverEntry\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	status = CreateDevice(pDriverObject);

	KdPrint(("DriverA: Leave A DriverEntry\n"));

	return status;
}

VOID OnTimerDpc(PKDPC pDpc, PVOID pContext, PVOID SysArg1, PVOID SysArg2)
{
	PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	PIRP currentPendingIRP = pdx->currentPendingIRP;

	KdPrint(("DriverA: complete the Driver A IRP_MJ_READ irp!\n"));

	currentPendingIRP->IoStatus.Status = STATUS_SUCCESS;
	currentPendingIRP->IoStatus.Information = 0;
	IoCompleteRequest(currentPendingIRP, IO_NO_INCREMENT);
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);
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

	KeInitializeTimer(&pDevExt->pollingTimer);

	KeInitializeDpc(&pDevExt->pollingDPC,
		OnTimerDpc,
		(PVOID)pDevObj);

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
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("DriverA:Enter A DriverUnload\n"));
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
	KdPrint(("DriverA:Leave A DriverUnload\n"));
}

NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("DriverA:Enter A HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
		pDevObj->DeviceExtension;

	//将IRP设置为挂起
	IoMarkIrpPending(pIrp);

	//将挂起的IRP记录下来
	pDevExt->currentPendingIRP = pIrp;

	//定义3秒后将IRP_MJ_READ的IRP完成
	ULONG ulMicroSecond = 3000000;

	//将32位整数转化成64位整数
	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMicroSecond);

	KeSetTimer(
		&pDevExt->pollingTimer,
		timeout,
		&pDevExt->pollingDPC);

	KdPrint(("DriverA:Leave A HelloDDKRead\n"));

	//返回pending状态
	return STATUS_PENDING;
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("DriverA:Enter A HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverA:Leave A HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
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

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
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
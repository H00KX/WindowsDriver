#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

#pragma LOCKEDCODE
VOID HelloDDKStartIo(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KIRQL oldirql;
	KdPrint(("Enter HelloDDKStartIo\n"));

	KIRQL currIrql = KeGetCurrentIrql();
	//获得cancel自旋锁
	IoAcquireCancelSpinLock(&oldirql);
	if (pIrp != pDeviceObject->CurrentIrp || pIrp->Cancel)
	{
		//如果当前有正在处理的IRP，则简单的入队列，病直接返回
		//入队列的工作由系统完成，在StartIo中不用负责
		IoReleaseCancelSpinLock(oldirql);
		KdPrint(("Leave HelloDDKStartIo\n"));
		return;
	}
	else
	{
		//由于正在处理该IRP，所以不允许调用取消例程
		//因此将此IRP的取消例程设置为NULL
		IoSetCancelRoutine(pIrp, NULL);
		IoReleaseCancelSpinLock(oldirql);
	}

	/*KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);*/

	//等待3秒
	//LARGE_INTEGER timeout;
	//timeout.QuadPart = -3 * 1000 * 10000;

	//定义一个3秒的演示，模拟IRP操作需要3秒时间
	//KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//在队列中读取一个IRP，并进行StartIo
	IoStartNextPacket(pDeviceObject, TRUE);

	KdPrint(("Leave HelloDDKStartIo\n"));
}

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
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	pDriverObject->DriverUnload = HelloDDKUnload;

	pDriverObject->DriverStartIo = HelloDDKStartIo;

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


NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutin\n"));
	
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//建立一个字符串数组与IRP类型对应起来
	static char* irpname[] =
	{
		"IRP_MJ_CREATE",
		"IRP_MJ_CREATE_NAMED_PIPE",
		"IRP_MJ_CLOSE",
		"IRP_MJ_READ",
		"IRP_MJ_WRITE",
		"IRP_MJ_QUERY_INFORMATION",
		"IRP_MJ_SET_INFORMATION",
		"IRP_MJ_QUERY_EA",
		"IRP_MJ_SET_EA",
		"IRP_MJ_FLUSH_BUFFERS",
		"IRP_MJ_QUERY_VOLUME_INFORMATION",
		"IRP_MJ_SET_VOLUME_INFORMATION",
		"IRP_MJ_DIRECTORY_CONTROL",
		"IRP_MJ_FILE_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CONTROL",
		"IRP_MJ_INTERNAL_DEVICE_CONTROL",
		"IRP_MJ_SHUTDOWN",
		"IRP_MJ_LOCK_CONTROL",
		"IRP_MJ_CLEANUP",
		"IRP_MJ_CREATE_MAILSLOT",
		"IRP_MJ_QUERY_SECURITY",
		"IRP_MJ_SET_SECURITY",
		"IRP_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP",
	};

	UCHAR type = stack->MajorFunction;
	if (type >= arraysize(irpname))
		KdPrint((" - Unknown IRP, major type %X\n", type));
	else
		KdPrint(("\t%s\n", irpname[type]));


	//对一般IRP的简单操作，后面会介绍对IRP更复杂的操作
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}

#pragma LOCKEDCODE
VOID OnCancelIRP(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter CancelReadIrp\n"));
	if (pIrp == pDeviceObject->CurrentIrp)
	{
		KIRQL oldirql = pIrp->CancelIrql;
		//释放Cancel自旋锁
		IoReleaseCancelSpinLock(pIrp->CancelIrql);

		IoStartNextPacket(pDeviceObject, TRUE);

		KeLowerIrql(oldirql);
	}
	else
	{
		//从设备队列中将该IRP抽取出来
		KeRemoveEntryDeviceQueue(&pDeviceObject->DeviceQueue, &pIrp->Tail.Overlay.DeviceQueueEntry);
		//释放Cancel自旋锁
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
	}

	//设置完成状态为 STATUS_CANCELLED
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave CancelReadIrp\n"));
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	//将IRP设置为挂起
	IoMarkIrpPending(pIrp);

	//将IRP插入系统的队列
	IoStartPacket(pDeviceObject, pIrp, 0, OnCancelIRP);

	KdPrint(("Leave HelloDDKRead\n"));

	return STATUS_SUCCESS;
}
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
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIoControl;


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

VOID WaitMicroSecond1_WaitForSingle(ULONG ulMircoSecond)
{
	KEVENT kEvent;

	KdPrint(("Thread suspends %d MircoSeconds....\n", ulMircoSecond));

	//初始化一个未激发的内核事件
	KeInitializeEvent(&kEvent, SynchronizationEvent, FALSE);

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMircoSecond);

	//讲过timeout后，线程继续运行
	KeWaitForSingleObject(&kEvent,
		Executive,
		KernelMode,
		FALSE,
		&timeout);
	KdPrint(("Thread is running again!\n"));
}

VOID WaitMicroSecond2_DelayExecution(ULONG ulMicroSecond)
{
	KdPrint(("Thread suspends %d MicroSeconds...\n", ulMicroSecond));

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMicroSecond);

	KeDelayExecutionThread(KernelMode, FALSE, &timeout);

	KdPrint(("Thread is running again!\n"));
}

VOID WaitMicroSecond3_StallExecutionProcessor(ULONG ulMicroSecond)
{
	KdPrint(("Thread suspends %d MicroSeconds...\n", ulMicroSecond));

	KeStallExecutionProcessor(ulMicroSecond);

	KdPrint(("Thread is running again, StallExecution\n"));
}

VOID WaitMicroSecond4(ULONG ulMicroSecond)
{
	KTIMER kTimer;  //内核计时器

	KeInitializeTimer(&kTimer);

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(ulMicroSecond * -10);

	//计时器不与DPC对象关联
	KeSetTimer(&kTimer, timeout, NULL);
	KdPrint(("Timer Thread suspends %d MicroSeconds...\n", ulMicroSecond));

	KeWaitForSingleObject(&kTimer, Executive, KernelMode, FALSE, NULL);

	KdPrint(("Thread is running again!\n"));
}

NTSTATUS HelloDDKDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Enter HelloDDKDeviceIoControl\n"));

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//得到传入信息
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	ULONG info = 0;

	ULONG ulMicroSecond = *(PULONG)pIrp->AssociatedIrp.SystemBuffer;

	switch (code)
	{
	case IOCTL_WAIT_METHOD1:
	{
		KdPrint(("IOCTL_WAIT_METHOD1_WaitForSingle\n"));
		WaitMicroSecond1_WaitForSingle(ulMicroSecond);
		break;
	}
	case IOCTL_WAIT_METHOD2:
	{
		KdPrint(("Method2 线程直接睡眠---KeDelayExecutThread\n"));
		WaitMicroSecond2_DelayExecution(ulMicroSecond);
		break;
	}

	case IOCTL_WAIT_METHOD3:
	{
		KdPrint(("Method3 自旋等待\n"));
		WaitMicroSecond3_StallExecutionProcessor(ulMicroSecond);
		break;
	}

	case IOCTL_WAIT_METHOD4:
	{
		KdPrint(("Method4 时钟等待\n"));
		WaitMicroSecond4(ulMicroSecond);
		break;
	}

	default:
		status = STATUS_INVALID_VARIANT;

	}

	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDeviceIoControl\n"));


	return status;

}

#include "Driver.h"

#define DevName		L"\\Device\\MyDDKDevice"
#define SymLinkName	L"\\??\\HelloDDK"


VOID SystemThread(IN PVOID pContext)
{
	KdPrint(("Enter SystemThread\n"));
	PEPROCESS pEProcess = IoGetCurrentProcess();
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);
	KdPrint(("This thread run in %s process!\n", ProcessName));
	KdPrint(("Leave SystemThread\n"));

	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID MyProcessThread(PVOID pContext)
{
	KdPrint(("Enter MyProcessThread\n"));
	//得到当前进程
	PEPROCESS pEProcess = IoGetCurrentProcess();
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);
	KdPrint(("This thread run in %s process!\n", ProcessName));

	KdPrint(("Leave MyProcessThread\n"));

	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID CreateThread()
{
	HANDLE hSystemThread, hMyThread;
	//创建System进程的线程
	NTSTATUS status = PsCreateSystemThread(&hSystemThread, 0, NULL, NULL, NULL, SystemThread, NULL);

	//创建进程线程
	status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread, NULL);

}

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//设置卸载函数
	pDriverObject->DriverUnload = HelloDDKUnload;

	//设置派遣函数
	for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutin;
	//pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDWrite;
	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadDDKDispatchRoutin;
	//pDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = HelloDDKQueryInfomation;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	

	KdPrint(("Leave DriverEntry\n"));
	return status;
}


NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT	pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DevName);

	//创建设备
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&(UNICODE_STRING)devName,
		FILE_DEVICE_UNKNOWN,
		0, TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	//申请模拟文件的缓冲区
	pDevExt->buffer = (PUCHAR)ExAllocatePool(PagedPool, MAX_FILE_LENGTH);
	//设置模拟文件大小
	pDevExt->file_length = 0;

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
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;
		if (pDevExt->buffer)
		{
			ExFreePool(pDevExt->buffer);
			pDevExt->buffer = NULL;
		}

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

NTSTATUS ReadDDKDispatchRoutin(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	CreateThread();

	KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}

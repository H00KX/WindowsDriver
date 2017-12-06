#include "Driver.h"

#define DeviceName	L"\\Device\\MyDDKDevice"
#define LinkName	L"\\??\\HelloDDK"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverUnload = HelloDDKUnload;
	for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutin;

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIoControl;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	KdPrint(("Leave DriverEntry\n"));
	return status;
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

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

	//申请模拟文件的缓冲区
	pDevExt->buffer = (PUCHAR)ExAllocatePool(PagedPool, MAX_FILE_LENGTH);
	//设置模拟文件大小
	pDevExt->file_length = 0;

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

NTSTATUS HelloDDKDispatchRoutin(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
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

	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}

NTSTATUS HelloDDKDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Enter HelloDDKDeviceIoControl\n"));

	//得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//得到输入缓冲区大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	//得到输出缓冲区大小
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	//得到IOCTL码
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	ULONG info = 0;

	switch (code)
	{
	case READ_PORT:
	{
		KdPrint(("READ_PORT\n"));

		PULONG InputBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
		ULONG port = (ULONG)(*InputBuffer);
		InputBuffer++;
		UCHAR method = (UCHAR)(*InputBuffer);

		//操作输出缓冲区
		PULONG OutputBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

		if (method == 1)
		{
			*OutputBuffer = READ_PORT_UCHAR((PUCHAR)port);
		}
		else if (method == 2)
		{
			*OutputBuffer = READ_PORT_USHORT((PUSHORT)port);
		}
		else if (method == 4)
		{
			*OutputBuffer = READ_PORT_ULONG((PULONG)port);
		}

		//设置实际操作输出缓冲区长度
		info = 4;

		break;
	}
	case WRITE_PORT:
	{
		KdPrint(("WRITE_PORT\n"));
		//缓冲区方式IOCTL
		PULONG InputBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;
		ULONG port = (ULONG)(*InputBuffer);
		InputBuffer++;
		UCHAR method = (UCHAR)(*InputBuffer);
		InputBuffer++;
		ULONG value = (ULONG)(*InputBuffer);

		//操作输出缓冲区
		PULONG OutputBuffer = (PULONG)pIrp->AssociatedIrp.SystemBuffer;

		if (method == 1)
		{
			WRITE_PORT_UCHAR((PUCHAR)port, (UCHAR)value);
		}
		else if (method == 2)
		{
			WRITE_PORT_USHORT((PUSHORT)port, (USHORT)value);
		}
		else if (method == 4)
		{
			WRITE_PORT_ULONG((PULONG)port, (ULONG)value);
		}
		info = 0;
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



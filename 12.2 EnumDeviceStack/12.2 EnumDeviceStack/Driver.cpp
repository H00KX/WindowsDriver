#pragma once
#include "Driver.h"

#define DeviceName	L"\\Device\\MyDDKDevice"
#define LinkName	L"\\??\\HelloDDK"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//设置卸载函数
	pDriverObject->DriverUnload = HelloDDKUnload;

	for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
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

	pDevObj->Flags |= DO_DIRECT_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	//申请模拟文件的缓冲区
	pDevExt->buffer = (PUCHAR)ExAllocatePool(PagedPool, MAX_FILE_LENGTH);
	//设置模拟文件大小
	pDevExt->file_length = 0;

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

VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
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

NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
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


VOID GetDeviceObjectInfo(PDEVICE_OBJECT DevObj)
{
	POBJECT_HEADER ObjectHeader;
	POBJECT_HEADER_NAME_INFO ObjectNameInfo;

	if (DevObj == NULL)
	{
		KdPrint(("DebObj is NULL!\n"));
		return;
	}
	//得到对象头
	ObjectHeader = OBJECT_TO_OBJECT_HEADER(DevObj);
	if (ObjectHeader)
	{
		//查询设备名称并打印
		ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

		if (ObjectNameInfo && ObjectNameInfo->Name.Buffer)
		{
			KdPrint(("Driver Name:%wZ - Device Name:%wZ - Driver Address:0x%x - Device Address:0x%x\n",
				&DevObj->DriverObject->DriverName,
				&ObjectNameInfo->Name,
				DevObj->DriverObject,
				DevObj));
		}
		//对于没有名称的设备，则打印NULL
		else if (DevObj->DriverObject)
		{
			KdPrint(("Driver Name:%wZ - Device Name:%S - Driver Address:0x%x - Device Address:0x%x\n",
				&DevObj->DriverObject->DriverName,
				L"NULL",
				DevObj->DriverObject,
				DevObj));
		}
	}


}


VOID 
GetAttachedDeviceInfo(PDEVICE_OBJECT DevObj)
{
	PDEVICE_OBJECT DeviceObject;

	if (DevObj == NULL)
	{
		KdPrint(("DevObj is NULL!\n"));
		return;
	}

	DeviceObject = DevObj->AttachedDevice;

	while (DeviceObject)
	{
		KdPrint(("Attached Driver Name:%wZ, Attached Driver Address:0x%x, Attached DeviceAddress:0x%x\n",
			&DeviceObject->DriverObject->DriverName,
			DeviceObject->DriverObject,
			DeviceObject));
		DeviceObject = DeviceObject->AttachedDevice;
	}
}

PDRIVER_OBJECT
EnumDeviceStack(PWSTR pwszDeviceName)
{
	UNICODE_STRING DriverName;
	PDRIVER_OBJECT DriverObject = NULL;
	PDEVICE_OBJECT DeviceObject = NULL;

	RtlInitUnicodeString(&DriverName, pwszDeviceName);

	ObReferenceObjectByName(
		&DriverName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		(POBJECT_TYPE)IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&DriverObject
	);

	if (DriverObject == NULL)
	{
		return NULL;
	}

	DeviceObject = DriverObject->DeviceObject;

	while (DeviceObject)
	{
		GetDeviceObjectInfo(DeviceObject);

		//判断当前设备上是否又过滤驱动
		if (DeviceObject->AttachedDevice)
		{
			GetAttachedDeviceInfo(DeviceObject);
		}

		//进一步判断当前设备上VPB中的设备
		if (DeviceObject->Vpb && DeviceObject->Vpb->DeviceObject)
		{
			GetDeviceObjectInfo(DeviceObject->Vpb->DeviceObject);

			if (DeviceObject->Vpb->DeviceObject->AttachedDevice)
			{
				GetAttachedDeviceInfo(DeviceObject->Vpb->DeviceObject);
			}
		}
		//得到建立在此驱动上的下一个设备 DEVICE_OBJECT
		DeviceObject = DeviceObject->NextDevice;
	}
	return DriverObject;
}


NTSTATUS HelloDDKDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Enter HelloDDKDeviceIoControl\n"));

	//得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//得到输入缓冲区大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	ULONG info = 0;

	switch (code)
	{
	case IOCTL_DUMP_DEVICE_STACK:
	{
		KdPrint(("IOCTL_DUMP_DEVICE_STACK\n"));
		WCHAR* InputBuffer = (WCHAR*)pIrp->AssociatedIrp.SystemBuffer;
		KdPrint(("%ws\n", InputBuffer));

		EnumDeviceStack(InputBuffer);

		break;
	}
	default:
		status = STATUS_INVALID_VARIANT;
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDeviceIoControl\n"));

	return status;
}

#include "SrcDrv.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"


#define TargetDevName L"\\Device\\TargetDevice"
#define TargetSymLinkName L"\\??\\TargetDDK"

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	NTSTATUS status;
	KdPrint(("Source: Enter Source DriverEntry\n"));

	//注册其他驱动调用函数入口
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;

	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;


	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	KdPrint(("Source: Leave Source DriverEntry\n\n"));
	return status;
}


NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
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
	KdPrint(("Source: Enter Source DriverUnload\n"));
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

	KdPrint(("Source: Leave Source DriverUnload\n"));
}


NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
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

	//KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}

VOID CompleteDriverA_Read(PVOID context, PIO_STATUS_BLOCK pStatus_block, ULONG)
{
	KdPrint(("Source: The Target Read completed now!\n"));
	KeSetEvent((PKEVENT)context, IO_NO_INCREMENT, FALSE);
}


NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Source: Enter Source HelloDDKRead\n"));

	//初始化Target的设备名
	UNICODE_STRING ustrTargetDevName;
	RtlInitUnicodeString(&ustrTargetDevName, TargetDevName);

	OBJECT_ATTRIBUTES objectAttributes;
	InitializeObjectAttributes(&objectAttributes,
		&ustrTargetDevName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	HANDLE hDevice;
	IO_STATUS_BLOCK status_block;

	status = ZwCreateFile(&hDevice,
		FILE_READ_ATTRIBUTES,
		&objectAttributes,
		&status_block,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		0,
		NULL,
		0);

	

	LARGE_INTEGER offset = RtlConvertLongToLargeInteger(0);
	if (NT_SUCCESS(status))
	{
		KdPrint(("ZwCreateFile Success...\n"));
		status = ZwReadFile(hDevice, NULL, NULL, NULL, &status_block, NULL, 0, &offset, NULL);
	}

	UINT32 ustatus = status;

	if (status == STATUS_PENDING)
	{
		KdPrint(("Source:ZwReadFile return STATUS_PENDING!\n"));

		PFILE_OBJECT FileObj;
		status = ObReferenceObjectByHandle(hDevice, EVENT_MODIFY_STATE, *ExEventObjectType, KernelMode, (PVOID*)&FileObj, NULL);

		if (NT_SUCCESS(status))
		{
			KdPrint(("Source: Waiting...\n"));
			KeWaitForSingleObject(&FileObj->Event, Executive, KernelMode, FALSE, NULL);
			ObDereferenceObject(FileObj);
		}
	}
	else
	{
		KdPrint(("Source: ZwRead 没有被挂起...\n"));
	}

	if (ustatus == STATUS_SUCCESS)
	{
		KdPrint(("Source: Success..\n"));
	}

	ZwClose(hDevice);

	status = STATUS_SUCCESS;
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	//将IRP请求结束
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Source: Leave HelloDDKRead\n"));

	return status;
}
NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	//KdPrint(("Source:Enter Source HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//KdPrint(("Source:Leave Source HelloDDKCreate\n"));

	return ntStatus;
}
NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	//KdPrint(("Source:Enter Source HelloDDKClose\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//KdPrint(("Source:Leave Source HelloDDKClose\n"));

	return ntStatus;
}
#pragma once
#include "TargetDrv.h"
#define DevName L"\\Device\\TargetDevice"
#define SymLinkName L"\\??\\TargetDDK"


extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("==========Taget: Enter Taget DirverEntry=============\n"));

	//ע�������������ú������
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	pDriverObject->DriverUnload = HelloDDKUnload;


	//���������豸����
	status = CreateDevice(pDriverObject);

	KdPrint(("==========Taget: Leave Taget DirverEntry=============\n"));

	return status;
}

#pragma LOCKEDCODE
VOID OnTimerDpc(IN PKDPC pDpc,
	IN PVOID pContext,
	IN PVOID SysArg1,
	IN PVOID SysArg2)
{
	PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	PIRP currentPendingIRP = pdx->currentPendingIRP;

	KdPrint(("Target:complete the Driver Target IRP_MJ_READ irp!\n"));

	//�������״̬ΪSTATUS_CANCELLED
	currentPendingIRP->IoStatus.Status = STATUS_SUCCESS;
	currentPendingIRP->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(currentPendingIRP, IO_NO_INCREMENT);
}

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//�����豸����
	UNICODE_STRING devName;

	RtlInitUnicodeString(&devName, DevName);

	//�����豸
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
	KdPrint(("1 Timer: %08X,   DPC: %08X\n", &pDevExt->pollingTimer, &pDevExt->pollingDPC));
	KeInitializeTimer(&pDevExt->pollingTimer);

	KeInitializeDpc(&pDevExt->pollingDPC, OnTimerDpc, (PVOID)pDevObj);

	KdPrint(("2 Timer: %08X,   DPC: %08X\n", &pDevExt->pollingTimer, &pDevExt->pollingDPC));
	//������������
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
	KdPrint(("Target: Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)                         //�����豸���󣬲�ɾ��
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//ɾ����������
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}

	KdPrint(("Target: Leave DriverUnload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	/*
	KdPrint(("Enter HelloDDKDispatchRoutin\n"));

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//����һ���ַ���������IRP���Ͷ�Ӧ����
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

		*/
	//��һ��IRP�ļ򵥲������������ܶ�IRP�����ӵĲ���
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//KdPrint(("Leave HelloDDKDispatchRoutin\n"));

	return status;
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_PENDING;
	KdPrint(("Target: ++++++++++++++Enter Target HelloDDKRead++++++++++++\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
		pDevObj->DeviceExtension;

	//��IRP����Ϊ����
	IoMarkIrpPending(pIrp);

	//�������IRP��¼����
	pDevExt->currentPendingIRP = pIrp;

	//����3���IRP_MJ_READ��IRP���
	ULONG ulMicroSecond = 3000000;

	//��32λ����ת����64λ����
	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMicroSecond);

	BOOLEAN bIs = KeSetTimer(
		&pDevExt->pollingTimer,
		timeout,
		&pDevExt->pollingDPC);
	if (bIs == TRUE)
	{
		KdPrint(("+++++++++++++++++Target: KeSetTimer is success+++++++++++++++\n"));
	}
	else
	{
		KdPrint(("+++++++++++++++++++++++++++++++Target: KeSetTimer is unsuccess\n"));
	}

	KdPrint(("Target:Leave Target HelloDDKRead\n"));

	

	return STATUS_PENDING;
}

NTSTATUS HelloDDKCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Target:Enter Target HelloDDKCreate\n"));
	
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKCreate\n"));
	
	return status;
}

NTSTATUS HelloDDKClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("Target:Enter Target HelloDDKClose\n"));
	
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKClose\n"));
	return status;
}


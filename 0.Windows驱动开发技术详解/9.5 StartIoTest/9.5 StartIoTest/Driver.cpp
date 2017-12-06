#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

#pragma LOCKEDCODE
VOID HelloDDKStartIo(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KIRQL oldirql;
	KdPrint(("Enter HelloDDKStartIo\n"));

	KIRQL currIrql = KeGetCurrentIrql();
	//���cancel������
	IoAcquireCancelSpinLock(&oldirql);
	if (pIrp != pDeviceObject->CurrentIrp || pIrp->Cancel)
	{
		//�����ǰ�����ڴ����IRP����򵥵�����У���ֱ�ӷ���
		//����еĹ�����ϵͳ��ɣ���StartIo�в��ø���
		IoReleaseCancelSpinLock(oldirql);
		KdPrint(("Leave HelloDDKStartIo\n"));
		return;
	}
	else
	{
		//�������ڴ����IRP�����Բ��������ȡ������
		//��˽���IRP��ȡ����������ΪNULL
		IoSetCancelRoutine(pIrp, NULL);
		IoReleaseCancelSpinLock(oldirql);
	}

	/*KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);*/

	//�ȴ�3��
	//LARGE_INTEGER timeout;
	//timeout.QuadPart = -3 * 1000 * 10000;

	//����һ��3�����ʾ��ģ��IRP������Ҫ3��ʱ��
	//KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	//�ڶ����ж�ȡһ��IRP��������StartIo
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

	//ע�������������ú������
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	pDriverObject->DriverUnload = HelloDDKUnload;

	pDriverObject->DriverStartIo = HelloDDKStartIo;

	//���������豸����
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
	KdPrint(("Enter DriverUnload\n"));
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
}


NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
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


	//��һ��IRP�ļ򵥲������������ܶ�IRP�����ӵĲ���
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
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
		//�ͷ�Cancel������
		IoReleaseCancelSpinLock(pIrp->CancelIrql);

		IoStartNextPacket(pDeviceObject, TRUE);

		KeLowerIrql(oldirql);
	}
	else
	{
		//���豸�����н���IRP��ȡ����
		KeRemoveEntryDeviceQueue(&pDeviceObject->DeviceQueue, &pIrp->Tail.Overlay.DeviceQueueEntry);
		//�ͷ�Cancel������
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
	}

	//�������״̬Ϊ STATUS_CANCELLED
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave CancelReadIrp\n"));
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	//��IRP����Ϊ����
	IoMarkIrpPending(pIrp);

	//��IRP����ϵͳ�Ķ���
	IoStartPacket(pDeviceObject, pIrp, 0, OnCancelIRP);

	KdPrint(("Leave HelloDDKRead\n"));

	return STATUS_SUCCESS;
}
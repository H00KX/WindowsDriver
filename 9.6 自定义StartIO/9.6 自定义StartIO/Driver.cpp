#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

#pragma LOCKEDCODE
VOID 
MyStartIo(PDEVICE_OBJECT pDeviceObject, PIRP pFistIrp)
{
	KdPrint(("Enter MyStartIo\n"));
	KIRQL oldirql;
	IoAcquireCancelSpinLock(&oldirql);

	KIRQL curIrp = KeGetCurrentIrql();
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	PKDEVICE_QUEUE_ENTRY device_entry;
	PIRP pIrp = pFistIrp;

	do
	{
		KEVENT event;
		KeInitializeEvent(&event, NotificationEvent, FALSE);

		//��3��
		//LARGE_INTEGER timeout;
		//timeout.QuadPart = -3 * 1000 * 1000 * 10;

		//KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

		KdPrint(("Complete a irp:%x\n", pIrp));
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		device_entry = KeRemoveDeviceQueue(&pDevExt->device_queue);
		KdPrint(("device_entry:%x\n", device_entry));
		if (device_entry == NULL)
		{
			break;
		}
		pIrp = CONTAINING_RECORD(device_entry, IRP, Tail.Overlay.DeviceQueueEntry);
	} while (1);
	IoReleaseCancelSpinLock(oldirql);

	KdPrint(("Leave MyStartIo\n"));
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

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;


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

	pDevExt->device_queue.DeviceListHead.Blink = pDevExt->device_queue.DeviceListHead.Flink = &pDevExt->device_queue.DeviceListHead;

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

VOID 
OnCancelIrp(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	KdPrint(("Enter CancelReadIrp\n"));

	//�ͷ�Cancel������
	IoReleaseCancelSpinLock(pIrp->CancelIrql);

	//�������״̬ΪSTATUS_CANCELLED
	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave CancelReadIrp\n"));
}

#pragma LOCKEDCODE
NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Enter HeeloDDKRead\n"));
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	LIST_ENTRY list;

	//��IRP����Ϊ����
	IoMarkIrpPending(pIrp);

	IoSetCancelRoutine(pIrp, OnCancelIrp);

	KIRQL curIrp = KeGetCurrentIrql();
	

	KIRQL oldirql;
	KeRaiseIrql(DISPATCH_LEVEL, &oldirql);

	curIrp = KeGetCurrentIrql();

	KdPrint(("DeviceQueueEntry:%x\n", &pIrp->Tail.Overlay.DeviceQueueEntry));
	if (!KeInsertDeviceQueue(&pDevExt->device_queue, &pIrp->Tail.Overlay.DeviceQueueEntry))
		MyStartIo(pDevObj, pIrp);

	KeLowerIrql(oldirql);

	KdPrint(("Leave HelloDDKRead\n"));
	list;
	//����pending״̬
	return STATUS_PENDING;
}

NTSTATUS HelloDDKCleanup(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{

	return STATUS_SUCCESS;
}


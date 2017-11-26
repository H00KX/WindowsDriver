#include "Driver.h"
#include "Ioctls.h"

#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"


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
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIoControl;

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	//���������豸����
	status = CreateDevice(pDriverObject);

	KdPrint(("DriverEntry end\n"));
	return status;
}

#pragma LOCKEDCODE
VOID OnTimerDpc(PKDPC pDpc,
	PVOID pContext,
	PVOID SysArg1,
	PVOID SysArg2)
{
	PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	PIRP currentPendingIrp = pdx->currentPendingIRP;

	KdPrint(("Cancel the current pending irp!\n"));

	//�������״̬Ϊ STATUS_CANCELLED
	currentPendingIrp->IoStatus.Status = STATUS_CANCELLED;
	currentPendingIrp->IoStatus.Information = 0;
	IoCompleteRequest(currentPendingIrp, IO_NO_INCREMENT);
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

	KeInitializeTimer(&pDevExt->pollingTimer);

	KeInitializeDpc(&pDevExt->pollingDPC,
		OnTimerDpc,
		(PVOID)pDevObj);

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

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
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

VOID Time_Test()
{
	LARGE_INTEGER current_system_time;

	KeQuerySystemTime(&current_system_time);

	LARGE_INTEGER current_local_time;
	ExSystemTimeToLocalTime(&current_system_time, &current_local_time);

	
	//�ɵ���ʱ��ʱ��õ���������Ϣ
	TIME_FIELDS current_time_info;
	RtlTimeToTimeFields(&current_local_time, &current_time_info);

	//��ʾ��������Ϣ
	KdPrint(("Current year: %d\n", current_time_info.Year));
	KdPrint(("Current month:%d\n", current_time_info.Month));
	KdPrint(("Current day:%d\n", current_time_info.Day));
	KdPrint(("Current Hour:%d\n", current_time_info.Hour));
	KdPrint(("Current Minute:%d\n", current_time_info.Minute));
	KdPrint(("Current Second:%d\n", current_time_info.Milliseconds));
	KdPrint(("Current Weekday:%d\n", current_time_info.Weekday));
}

NTSTATUS	HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("Enter HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	//��IRP����Ϊ����
	IoMarkIrpPending(pIrp);

	//�������IRP��¼����
	pDevExt->currentPendingIRP = pIrp;

	//����3��ĳ�ʱ
	ULONG ulMicroSecond = 3000000;

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMicroSecond);

	KeSetTimer(&pDevExt->pollingTimer,
		timeout,
		&pDevExt->pollingDPC);

	KdPrint(("Leave HelloDDKRead\n"));

	NTSTATUS status = STATUS_PENDING;

	return status;
}

NTSTATUS HelloDDKDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Enter HelloDDKDeviceIoControl\n"));

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//�õ����������Ϣ
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	ULONG info = 0;

	switch (code)
	{
	case IOCTL_SYSTEM_TIME:
	{
		KdPrint(("IOCTL_TIME_TEST\n"));
		Time_Test();
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


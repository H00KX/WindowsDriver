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

	//ע�������������ú������
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKDeviceIoControl;


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

VOID WaitMicroSecond1_WaitForSingle(ULONG ulMircoSecond)
{
	KEVENT kEvent;

	KdPrint(("Thread suspends %d MircoSeconds....\n", ulMircoSecond));

	//��ʼ��һ��δ�������ں��¼�
	KeInitializeEvent(&kEvent, SynchronizationEvent, FALSE);

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMircoSecond);

	//����timeout���̼߳�������
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
	KTIMER kTimer;  //�ں˼�ʱ��

	KeInitializeTimer(&kTimer);

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(ulMicroSecond * -10);

	//��ʱ������DPC�������
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
	//�õ�������Ϣ
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
		KdPrint(("Method2 �߳�ֱ��˯��---KeDelayExecutThread\n"));
		WaitMicroSecond2_DelayExecution(ulMicroSecond);
		break;
	}

	case IOCTL_WAIT_METHOD3:
	{
		KdPrint(("Method3 �����ȴ�\n"));
		WaitMicroSecond3_StallExecutionProcessor(ulMicroSecond);
		break;
	}

	case IOCTL_WAIT_METHOD4:
	{
		KdPrint(("Method4 ʱ�ӵȴ�\n"));
		WaitMicroSecond4(ulMicroSecond);
		break;
	}

	default:
		status = STATUS_INVALID_VARIANT;

	}

	//���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDeviceIoControl\n"));


	return status;

}

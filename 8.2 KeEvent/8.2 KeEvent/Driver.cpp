#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

#define IOCTL_TRANSMIT_EVENT CTL_CODE(FILE_DEVICE_UNKNOWN, \
	0x801, \
	METHOD_BUFFERED, \
	FILE_ANY_ACCESS)

/*****************************************************************************************/
//�ں�ģʽ�µ� event
VOID MyProcessThread(IN PVOID pContext)
{
	PKEVENT pEvent = (PKEVENT)pContext;
	KdPrint(("Enter MyProcessThread\n"));

	//�����¼�
	KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);

	KdPrint(("Leave MyProcessThread\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}
VOID Test()
{
	HANDLE hMyThread;
	KEVENT kEvent;

	//��ʼ���ں˶���
	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	NTSTATUS status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread, &kEvent);

	KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
}

/******************************************************************************************/
//����ģʽ�µ� event

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//ע�������������ú������
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

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
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("Enter HelloDDKDeviceIoControl\n"));
	//��õ�ǰ��ջ
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//���������������������С
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//�õ�IOCTL��
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG info = 0;
	switch (code)
	{
	case IOCTL_TRANSMIT_EVENT:
	{
		KdPrint(("IOCTL_TEST1\n"));
		//�õ�Ӧ�ó��򴫵ݽ������¼�
		HANDLE hUserEvent = *(HANDLE*)pIrp->AssociatedIrp.SystemBuffer;
		PKEVENT pEvent;
		status = ObReferenceObjectByHandle(hUserEvent,EVENT_MODIFY_STATE,*ExEventObjectType,
			KernelMode,
			(PVOID*)&pEvent,
			NULL);
		//�����¼�
		KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
		//��С���ü���
		ObDereferenceObject(pEvent);

		break;
	}
	default:
		status = STATUS_INVALID_VARIANT;
	}
	//����IRP���״̬
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	//����IRP����
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDeviceIOControl\n"));
	return status;
}

#include "Driver.h"

VOID ProbeTest()
{
	PVOID badPointer = NULL;

	KdPrint(("Enter ProberTest\n"));

	__try
	{
		KdPrint(("Enter __try block\n"));

		//�жϿ�ָ���Ƿ�ɶ�����Ȼ�ᵼ���쳣
		ProbeForWrite(badPointer, 100, 4);

		KdPrint(("Leave __try block\n"));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("Catch the exception\n"));
		KdPrint(("The program will keep going\n"));
	}

	KdPrint(("Leave ProbeTest\n"));
}

NTSTATUS Foo(PCHAR* str)
{
	ASSERT(str != NULL); //����

	return STATUS_SUCCESS;
}
NTSTATUS Foo2(PCHAR* str)
{
	if (str == NULL)
	{
		return STATUS_INVALID_VARIANT;
	}
	return STATUS_SUCCESS;
}

NTSTATUS TryFinallyTest()
{
	NTSTATUS status = STATUS_SUCCESS;
	__try
	{
		//DO SOMETHING
		return STATUS_SUCCESS;
	}
	__finally
	{
		KdPrint(("Enter finally block\n"));
		return status;
	}
}



extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath)
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

	ProbeTest();

	TryFinallyTest();

	KdPrint(("DriverEntry end\n"));
	return status;
}

/************************************************************************
* ��������:CreateDevice
* ��������:��ʼ���豸����
* �����б�:
pDriverObject:��I/O�������д���������������
* ���� ֵ:���س�ʼ��״̬
*************************************************************************/

NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT	pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//�����豸����
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

	//�����豸
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
	//������������
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

/************************************************************************
* ��������:HelloDDKUnload
* ��������:�������������ж�ز���
* �����б�:
pDriverObject:��������
* ���� ֵ:����״̬
*************************************************************************/

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;

		//ɾ����������
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
}

/************************************************************************
* ��������:HelloDDKDispatchRoutine
* ��������:�Զ�IRP���д���
* �����б�:
pDevObj:�����豸����
pIrp:��IO�����
* ���� ֵ:����״̬
*************************************************************************/

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

#include <ntddk.h>

#define		DEVICE_NAME		L"\\device\\ntmodeldrv"
#define		LINK_NAME		L"\\dosdevices\\ntmodeldrv"

//��������� R3��R0 ��һ��
#define IOCTRL_BASE		0x800

#define	MYIOCTRL_CODE(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)
/*
FILE_DEVICE_UNKONOWN: ��������
IOCTRL_BASE+i  : ������
METHOD_BUFFERED: ͨѶ��ʽ
FILE_ANY_ACCESS������Ȩ��
*/

#define CTL_HELLO	MYIOCTRL_CODE(0)
#define CTL_PRINT	MYIOCTRL_CODE(1)
#define CTL_BYE		MYIOCTRL_CODE(2)

NTSTATUS DispatchCommon(PDEVICE_OBJECT pObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT pObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT pObject, PIRP pIrp)
{
	PVOID pReadBuffer = NULL;
	ULONG uReadLength = 0;
	PIO_STACK_LOCATION pStack = NULL;
	ULONG uMin = 0;
	ULONG uHelloStr = 0;

	uHelloStr = ((wcslen("hello world") + 1) * sizeof(WCHAR));

	//1. ��ȡ����ͳ���
	pReadBuffer = pIrp->AssociatedIrp.SystemBuffer;
	pStack = IoGetCurrentIrpStackLocation(pIrp);       //��ȡ��ǰIRP��ջ
	uReadLength = pStack->Parameters.Read.Length;		//���Ļ��泤��, R3����������

	//2. ��д����
	uMin = uReadLength > uHelloStr ? uHelloStr : uReadLength;
	RtlCopyMemory(pReadBuffer, L"hello world", uMin);

	//3. ���IRP
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uMin;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchWrite(PDEVICE_OBJECT pObject, PIRP pIrp)
{
	PVOID pWriteBuffer = NULL;
	ULONG uWriteLength = 0;
	PIO_STACK_LOCATION pStack = NULL;

	PVOID pBuffer = NULL;

	pWriteBuffer = pIrp->AssociatedIrp.SystemBuffer;

	//2. ��ȡ��д�ĳ���
	pStack = IoGetCurrentIrpStackLocation(pIrp);
	uWriteLength = pStack->Parameters.Write.Length;
	
	//3.���ں��з����ڴ�
	pBuffer = ExAllocatePoolWithTag(
		PagedPool,
		uWriteLength,
		'TSET'
	);

	if (pBuffer == NULL)
	{
		pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	memset(pBuffer, 0, uWriteLength);
	RtlCopyMemory(pBuffer, pWriteBuffer, uWriteLength);

	ExFreePool(pBuffer);
	pBuffer = NULL;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uWriteLength;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//������
NTSTATUS DispatchIoctrl(PDEVICE_OBJECT pObject, PIRP pIrp)
{
	ULONG uIoctrlCode = 0;
	PVOID pInputBuff = NULL;
	PVOID pOutputBuff = NULL;

	ULONG uInputLength = 0;
	ULONG uOutputLength = 0;
	PIO_STACK_LOCATION pStack = NULL;

	pInputBuff = pOutputBuff = pIrp->AssociatedIrp.SystemBuffer;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	uInputLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutputLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	uIoctrlCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	switch (uIoctrlCode)
	{
	case CTL_HELLO:
		DbgPrint(("Hello iocontrol\n"));
		break;
	case CTL_PRINT:
		KdPrint(("%wZ\n", pInputBuff));
		break;
	case CTL_BYE:
		DbgPrint(("Goodbye iocontrol\n"));
		break;
	default:
		DbgPrint(("Unknown iocontrol\n"));
	}
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = uInputLength;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchClean(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING uLinkName = { 0 };
	RtlInitUnicodeString(&uLinkName, LINK_NAME);
	IoDeleteSymbolicLink(&uLinkName);

	IoDeleteDevice(pDriverObject->DeviceObject);

	DbgPrint(("Driver unloadded\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	UNICODE_STRING uDeviceName = { 0 };
	UNICODE_STRING uLinkName = { 0 };
	NTSTATUS ntStatus = 0;
	PDEVICE_OBJECT pDeviceObject = NULL;
	ULONG i = 0;

	DbgPrint(("Driver load beging\n"));

	RtlInitUnicodeString(&uDeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&uLinkName, LINK_NAME);

	//����һ���豸�����������ڽ���IRP
	ntStatus = IoCreateDevice(
		pDriverObject,
		0,
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDeviceObject
	);

	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint(("IoCreateDevice failed:%x", ntStatus));
		return ntStatus;
	}
	//���豸��������ͨѶ��ʽ�ı�־
	pDeviceObject->Flags |= DO_BUFFERED_IO;

	//������������
	ntStatus = IoCreateSymbolicLink(&uLinkName, &uDeviceName);

	if (!NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice(pDeviceObject);
		DbgPrint(("IoCreateSymbolicLink failed: %x\n", ntStatus));
		return ntStatus;
	}

	//��ʼ���ַ�����
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
	{
		pDriverObject->MajorFunction[i] = DispatchCommon;
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_READ]	= DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]	= DispatchWrite;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctrl;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchClean;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;

	pDriverObject->DriverUnload = DriverUnload;

	DbgPrint(("Driver load ok!\n"));

	return STATUS_SUCCESS;
}


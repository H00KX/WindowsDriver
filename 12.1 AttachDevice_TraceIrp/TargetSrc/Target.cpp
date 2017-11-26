#include "Target.h"

#define DeviceName L"\\Device\\TargetDevice"
#define LinkName	L"\\??\\MyTargetDevice"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("Target: Enter Target DriverEntry\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	//���������豸����
	status = CreateDevice(pDriverObject);

	KdPrint(("Target: Leave Target DriverEntry\n"));
	return status;
}

VOID OnTimerDpc(PKDPC pDpc, PVOID pContext, PVOID SysArg1, PVOID SysArg2)
{
	PDEVICE_OBJECT pDevObj = (PDEVICE_OBJECT)pContext;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	PIRP currentPendingIRP = pdx->currentPendingIRP;

	KdPrint(("Target: complete the Driver a IRP_MJ_READ irp!\n"));

	currentPendingIRP->IoStatus.Status = STATUS_SUCCESS;
	currentPendingIRP->IoStatus.Information = 0;
	IoCompleteRequest(currentPendingIRP, IO_NO_INCREMENT);
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

	//�����豸
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	KeInitializeTimer(&pDevExt->pollingTimer);

	KeInitializeDpc(&pDevExt->pollingDPC, OnTimerDpc, (PVOID)pDevObj);

	//������������
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
	KdPrint(("Target:Enter Target DriverUnload\n"));
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
	KdPrint(("Target:Leave Target DriverUnload\n"));
}



NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Target: Enter Target HelloDDKRead\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	//��IRP����Ϊ����
	IoMarkIrpPending(pIrp);

	pDevExt->currentPendingIRP = pIrp;
	ULONG ulMicroSecond = 3000000;

	LARGE_INTEGER timeout = RtlConvertLongToLargeInteger(-10 * ulMicroSecond);

	KeSetTimer(
		&pDevExt->pollingTimer,
		timeout,
		&pDevExt->pollingDPC
	);

	KdPrint(("Target: Leave Target HelloDDKRead\n"));

	return STATUS_PENDING;
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Target:Enter Target HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Target:Enter Target HelloDDKCreate\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKCreate\n"));
	return status;
}

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Target:Enter Target HelloDDKClose\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Target:Leave Target HelloDDKClose\n"));
	return status;
}

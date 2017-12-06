#include <ntddk.h>
#include <ntddkbd.h>
#include "stdarg.h"
#include "stdio.h"
#include "KeyFilter.h"


#undef WIN2K

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Ctrl2capDispatchGeneral)
#if WIN2K
#pragma alloc_text(PAGE, Ctrl2capAddDevice)
#pragma alloc_text(PAGE, Ctrl2capUnload)
#pragma alloc_text(PAGE, Ctrl2capPnp)
#pragma alloc_text(PAGE, Ctrl2capPower)

#endif //WIN2K
#endif //ALLOC_PRAGMA

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,
	PUNICODE_STRING RegistryPaht)
{
	ULONG	i;
	DbgPrint(("Enter DriverEntry\n"));

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = Ctrl2capDispatchGeneral;
	}

	pDriverObject->MajorFunction[IRP_MJ_READ] = Ctrl2capDispatchRead;

#if WIN2K
	pDriverObject->MajorFunction[IRP_MJ_POWER] = Ctrl2capPower;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = Ctrl2capPnp;
	pDriverObject->DriverUnload = Ctrl2capUnload;
	pDriverObject->DriverExtension->AddDevice = Ctrl2capAddDevice;

	return STATUS_SUCCESS;
#else
	return Ctrl2capInit(pDriverObject);
#endif

}

NTSTATUS Ctrl2capInit(PDRIVER_OBJECT pDriverObject)
{
	CCHAR				ntNameBuffer[64];
	STRING				ntNameString;
	UNICODE_STRING		ntUnicodeString;
	PDEVICE_OBJECT		device;
	NTSTATUS			status;
	PDEVICE_EXTENSION	devExt;
	WCHAR				messageBuffer[] = L"Ctrl2cap Initialized\n";
	UNICODE_STRING		messageUnicodeString;

	sprintf(ntNameBuffer, "\\Device\\KeyboardClass0");
	RtlInitAnsiString(&ntNameString, ntNameBuffer);
	RtlAnsiStringToUnicodeString(&ntUnicodeString, &ntNameString, TRUE);

	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_KEYBOARD,
		0,
		FALSE,
		&device);

	if (!NT_SUCCESS(status))
	{
		DbgPrint(("Keyboard Create Device Failed..\n"));

		RtlFreeUnicodeString(&ntUnicodeString);

		return STATUS_SUCCESS;
	}

	RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

	devExt = (PDEVICE_EXTENSION)device->DeviceExtension;

	device->Flags |= DO_BUFFERED_IO;
	device->Flags &= ~DO_DEVICE_INITIALIZING;

	status = IoAttachDevice(device, &ntUnicodeString, &devExt->TopOfStack);
	
	if (!NT_SUCCESS(status))
	{
		DbgPrint(("Connect with keyboard failed!\n"));
		IoDeleteDevice(device);
		RtlFreeUnicodeString(&ntUnicodeString);
		return STATUS_SUCCESS;
	}

	RtlFreeUnicodeString(&ntUnicodeString);
	DbgPrint(("Successfully connected to keyboard device\n"));

	RtlInitUnicodeString(&messageUnicodeString, messageBuffer);

	ZwDisplayString(&messageUnicodeString);
	return STATUS_SUCCESS;
}

#if WIN2K
NTSTATUS
Ctrl2capAddDevice(PDRIVER_OBJECT pDriverObject,
	PDEVICE_OBJECT pdo)
{
	PDEVICE_EXTENSION		devExt;
	IO_ERROR_LOG_PACKET		errorLogEntry;
	PDEVICE_OBJECT			device;
	NTSTATUS				status = STATUS_SUCCESS;

	DbgPrint(("Ctrl2capAddDevice\n"));
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_KEYBOARD,
		0,
		FALSE,
		&device);

	if (!NT_SUCCESS(status))
		return status;

	RtlZeroMemory(device->DeviceExtension, sizeof(DEVICE_EXTENSION));

	devExt = (PDEVICE_EXTENSION)device->DeviceExtension;
	devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

	ASSERT(devExt->TopOfStack);

	device->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	device->Flags &= ~DO_DEVICE_INITIALIZING;

	return status;
}

NTSTATUS Ctrl2capPnp(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PDEVICE_EXTENSION		devExt;
	PIO_STACK_LOCATION		irpStack;
	NTSTATUS				status = STATUS_SUCCESS;
	KIRQL					oldIrql;
	KEVENT					event;

	devExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	irpStack = IoGetCurrentIrpStackLocation(pIrp);

	switch (irpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
	{
		IoSkipCurrentIrpStackLocation(pIrp);
		IoCallDriver(devExt->TopOfStack, pIrp);

		IoDetachDevice(devExt->TopOfStack);
		IoDeleteDevice(pDeviceObject);

		status = STATUS_SUCCESS;
		break;
	}

	case IRP_MN_SURPRISE_REMOVAL:
	{
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->TopOfStack, pIrp);
		break;
	}

	case IRP_MN_START_DEVICE:
	case IRP_MN_QUERY_REMOVE_DEVICE:
	case IRP_MN_QUERY_STOP_DEVICE:
	case IRP_MN_CANCEL_REMOVE_DEVICE:
	case IRP_MN_CANCEL_STOP_DEVICE:
	case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
	case IRP_MN_STOP_DEVICE:
	case IRP_MN_QUERY_DEVICE_RELATIONS:
	case IRP_MN_QUERY_INTERFACE:
	case IRP_MN_QUERY_CAPABILITIES:
	case IRP_MN_QUERY_DEVICE_TEXT:
	case IRP_MN_QUERY_RESOURCES:
	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
	case IRP_MN_READ_CONFIG:
	case IRP_MN_WRITE_CONFIG:
	case IRP_MN_EJECT:
	case IRP_MN_SET_LOCK:
	case IRP_MN_QUERY_ID:
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
	default:
	{
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->TopOfStack, pIrp);
		break;
	}
	}
	return status;
}

NTSTATUS Ctrl2capPower(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PDEVICE_EXTENSION devExt;
	devExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	PoStartNextPowerIrp(pIrp);

	IoSkipCurrentIrpStackLocation(pIrp);

	return PoCallDriver(devExt->TopOfStack, pIrp);
}

VOID Ctrl2capUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	ASSERT(NULL == Driver->DeviceObject);
}
#endif

NTSTATUS Ctrl2capReadComplete(PDEVICE_OBJECT DeviceObject,PIRP pIrp,PVOID Context)
{
	PIO_STACK_LOCATION		IrpSp;
	PKEYBOARD_INPUT_DATA	KeyData;

	int		numKeys, i;

	IrpSp = IoGetCurrentIrpStackLocation(pIrp);
	if (NT_SUCCESS(pIrp->IoStatus.Status))
	{
		KeyData = pIrp->AssociatedIrp.SystemBuffer;
		numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		for (i = 0; i < numKeys; ++i)
		{
			DbgPrint(("ScanCode: %x ", KeyData[i].MakeCode));
			DbgPrint(("%s\n", KeyData[i].Flags ? "Up" : "Down"));

			if (KeyData[i].MakeCode == CAPS_LOCK)
			{
				KeyData[i].MakeCode = LCONTROL;
			}
		}
	}

	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}
	return pIrp->IoStatus.Status;
}

NTSTATUS Ctrl2capDispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PDEVICE_EXTENSION devExt;
	PIO_STACK_LOCATION	currentIrpStack;
	PIO_STACK_LOCATION	nextIrpStack;

	devExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	nextIrpStack = IoGetNextIrpStackLocation(pIrp);

	*nextIrpStack = *currentIrpStack;

	IoSetCompletionRoutine(pIrp, Ctrl2capReadComplete, pDeviceObject, TRUE, TRUE, TRUE);
	
	return IoCallDriver(devExt->TopOfStack, pIrp);
}

NTSTATUS Ctrl2capDispatchGeneral(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
#if WIN2K
	IoSkipCurrentIrpStackLocation(pIrp);
#else
	pIrp->CurrentLocation++;
	pIrp->Tail.Overlay.CurrentStackLocation++;
#endif
	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->TopOfStack, pIrp);
}


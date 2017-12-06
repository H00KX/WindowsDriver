#include "HelloWDM.h"

#define DEVICE_NAME		L"\\Device\\MyWDMDevice"
#define LINK_NAME		L"\\DosDevices\\HelloWDM"

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverExtension->AddDevice = HelloWDMAddDevice;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = HelloWDMPnp;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
	pDriverObject->MajorFunction[IRP_MJ_READ] =
	pDriverObject->MajorFunction[IRP_MJ_CREATE] =
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloWDMDispatchRoutine;

	pDriverObject->DriverUnload = HelloWDMUnload;

	KdPrint(("Leave DriverEntry\n"));
	return STATUS_SUCCESS;
}

void Dump(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("----------------------------------------------\n"));
	KdPrint(("Begin Dump...\n"));
	KdPrint(("Driver Address:0X%08X\n", pDriverObject));
	KdPrint(("Driver name:%wZ\n", &pDriverObject->DriverName));
	KdPrint(("Driver HardwareDatabase:%wZ\n", pDriverObject->HardwareDatabase));
	KdPrint(("Driver first device:0X%08X\n", pDriverObject->DeviceObject));

	PDEVICE_OBJECT	pDevice = pDriverObject->DeviceObject;
	int i = 1;
	for (; pDevice != NULL; pDevice = pDevice->NextDevice)
	{
		KdPrint(("The %d device\n", i++));
		KdPrint(("Device AttachedDevice:0X%08X\n", pDevice->AttachedDevice));
		KdPrint(("Device NextDevice:0X%08X\n", pDevice->NextDevice));
		KdPrint(("Device StackSize:%d\n", pDevice->StackSize));
		KdPrint(("Device's DriverObject:%0X%08X\n", pDevice->DriverObject));
	}

	KdPrint(("Dump over!\n"));
	KdPrint(("--------------------------------------------------------------------\n"));
}

void DumpDeviceStack(IN PDEVICE_OBJECT pdo)
{
	KdPrint(("--------------------------------------\n"));
	KdPrint(("Begin Dump device stack...\n"));

	PDEVICE_OBJECT pDevice = pdo;
	int i = 0;
	for (; pDevice != NULL; pDevice = pDevice->AttachedDevice)
	{
		KdPrint(("The %d device in device stack\n", i++));
		KdPrint(("Device AttachedDevice:0X%08X\n", pDevice->AttachedDevice));
		KdPrint(("Device NextDevice: 0X%08X\n", pDevice->NextDevice));
		KdPrint(("Device StackSize: %d\n", pDevice->StackSize));
		KdPrint(("Device's DriverObject:0X%08X\n", pDevice->DriverObject));
	}
	KdPrint(("Dump over!\n"));
	KdPrint(("--------------------------------------------------\n"));
}

NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	KdPrint(("Enter HelloWDMAddDevice\n"));

	NTSTATUS status;
	PDEVICE_OBJECT	fdo;
	UNICODE_STRING	devName;
	RtlInitUnicodeString(&devName, DEVICE_NAME);
	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&fdo
	);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	pdx->fdo = fdo;
	pdx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);

	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, LINK_NAME);

	pdx->ustrDeviceName = devName;
	pdx->ustrSymLinkName = symLinkName;

	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&pdx->ustrSymLinkName);
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status))
		{
			IoDeleteDevice(fdo);
			return status;
		}
	}
	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	Dump(DriverObject);
	DumpDeviceStack(PhysicalDeviceObject);

	KdPrint(("Leave HellWDMAddDevice\n"));
	return STATUS_SUCCESS;
}

NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	KdPrint(("Enter DefaultPnpHandler\n"));
	IoSkipCurrentIrpStackLocation(Irp);
	KdPrint(("Leave DefaultPnpHandler\n"));
	
	return IoCallDriver(pdx->NextStackDevice, Irp);
}

NTSTATUS HandleRemoveDevice(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	KdPrint(("Enter HandleRemoveDevice\n"));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, Irp);
	IoDeleteSymbolicLink(&pdx->ustrSymLinkName);

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	//É¾³ýfdo
	IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave HandleRemoveDevice\n"));
	return status;
}

NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT fdo,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloWDMPnp\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP Irp) =
	{
		DefaultPnpHandler,		// IRP_MN_START_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_REMOVE_DEVICE
		HandleRemoveDevice,		// IRP_MN_REMOVE_DEVICE
		DefaultPnpHandler,		// IRP_MN_CANCEL_REMOVE_DEVICE
		DefaultPnpHandler,		// IRP_MN_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_CANCEL_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_RELATIONS
		DefaultPnpHandler,		// IRP_MN_QUERY_INTERFACE
		DefaultPnpHandler,		// IRP_MN_QUERY_CAPABILITIES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_TEXT
		DefaultPnpHandler,		// IRP_MN_FILTER_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// 
		DefaultPnpHandler,		// IRP_MN_READ_CONFIG
		DefaultPnpHandler,		// IRP_MN_WRITE_CONFIG
		DefaultPnpHandler,		// IRP_MN_EJECT
		DefaultPnpHandler,		// IRP_MN_SET_LOCK
		DefaultPnpHandler,		// IRP_MN_QUERY_ID
		DefaultPnpHandler,		// IRP_MN_QUERY_PNP_DEVICE_STATE
		DefaultPnpHandler,		// IRP_MN_QUERY_BUS_INFORMATION
		DefaultPnpHandler,		// IRP_MN_DEVICE_USAGE_NOTIFICATION
		DefaultPnpHandler,		// IRP_MN_SURPRISE_REMOVAL
	};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
	{
		status = DefaultPnpHandler(pdx, pIrp);
		return status;
	}

#if DBG
	static char* fcnname[] = 
	{
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL"
	};

	KdPrint(("PNP Request (%s)\n", fcnname[fcn]));
#endif

	status = (*fcntab[fcn])(pdx, pIrp);
	KdPrint(("Leave HelloWDMPnp"));
	return status;
}

NTSTATUS HelloWDMDispatchRoutine(IN PDEVICE_OBJECT fdo,
	IN PIRP Irp)
{
	KdPrint(("Enter HelloWDMDispatchRoutine\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;	// no bytes xfered
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloWDMDispatchRoutine\n"));
	return STATUS_SUCCESS;
}

void HelloWDMUnload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("Enter HelloWDMUnload\n"));
	KdPrint(("Leave HelloWDMUnload\n"));
}



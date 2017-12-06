#include "HelloWDM.h"
#include <initguid.h>
#include "guid.h"


extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("Enter DriverEntry!\n"));

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloWDMDispatchRoutine;
	}
	pDriverObject->DriverExtension->AddDevice = HelloWDMAddDevice;
	pDriverObject->DriverUnload = HelloWDMUnload;

	KdPrint(("Leave DriverEntry!\n"));
	return STATUS_SUCCESS;
}

NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	KdPrint(("Enter HelloWDMAddDevice\n"));

	NTSTATUS status;
	PDEVICE_OBJECT fdo;
	status = IoCreateDevice(DriverObject,
		sizeof(DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&fdo);
	if (!NT_SUCCESS(status))
		return status;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	pdx->fdo = fdo;
	pdx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);

	//创建设备接口
	status = IoRegisterDeviceInterface(PhysicalDeviceObject, 
		&MY_WDM_DEVICE, 
		NULL, 
		&pdx->interfaceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	KdPrint(("%wZ\n", &pdx->interfaceName));
	IoSetDeviceInterfaceState(&pdx->interfaceName, TRUE);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	KdPrint(("Leave HelloWDMAddDevice\n"));

	return STATUS_SUCCESS;
}

NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	KdPrint(("Enter DefaultPnpHandler\n"));
	IoSkipCurrentIrpStackLocation(pIrp);
	KdPrint(("Leave DefaultPnpHandler\n"));
	return IoCallDriver(pdx->NextStackDevice, pIrp);
}

NTSTATUS HandleRemoveDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);

	IoSetDeviceInterfaceState(&pdx->interfaceName, FALSE);
	RtlFreeUnicodeString(&pdx->interfaceName);

	//剥离附加的设备
	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	//删除fdo
	IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave HandlRemoveDevice\n"));

	return status;
}

NTSTATUS HandleStartDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	KdPrint(("Enter HandleStartDevice\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);

	KdPrint(("Leave HandleStartDevice\n"));
	return status;
}

NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT fdo, IN PIRP pIrp)
{
	KdPrint(("Enter HelloWDMPnp\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP pIrp) = 
	{
		HandleStartDevice,		// IRP_MN_START_DEVICE
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
	{						// 未知的子功能代码
		status = DefaultPnpHandler(pdx, pIrp); // some function we don't know about
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
		"IRP_MN_SURPRISE_REMOVAL",
	};

	KdPrint(("PNP Request (%s)\n", fcnname[fcn]));
#endif // DBG

	status = (*fcntab[fcn])(pdx, pIrp);
	KdPrint(("Leave HelloWDMPnp\n"));
	return status;
}

NTSTATUS HelloWDMDispatchRoutine(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
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


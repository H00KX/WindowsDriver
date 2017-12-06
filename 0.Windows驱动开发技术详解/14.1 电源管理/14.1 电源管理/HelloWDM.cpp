#include "HelloWDM.h"

#include <initguid.h>
#include "guid.h"

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverExtension->AddDevice = HelloWDMAddDevice;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = HelloWDMPnp;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
		pDriverObject->MajorFunction[IRP_MJ_CREATE] =
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] =
		pDriverObject->MajorFunction[IRP_MJ_READ] =
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloWDMDispatchRoutine;
	pDriverObject->DriverUnload = HelloWDMUnload;

	KdPrint(("Leave DriverEntry\n"));
	return STATUS_SUCCESS;
}

NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	KdPrint(("Enter HelloWDMAddDevice\n"));

	NTSTATUS status;
	PDEVICE_OBJECT fdo;
	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),
		NULL,//没有指定设备名
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
	status = IoRegisterDeviceInterface(PhysicalDeviceObject, &MY_WDM_DEVICE, NULL, &pdx->interfaceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	KdPrint(("%wZ\n", &pdx->interfaceName));
	IoSetDeviceInterfaceState(&pdx->interfaceName, TRUE);

	if (!NT_SUCCESS(status))
	{
		if (!NT_SUCCESS(status))
		{
			return status;
		}
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
	KdPrint(("Enter HandleRemoveDevice\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);

	IoSetDeviceInterfaceState(&pdx->interfaceName, FALSE);
	RtlFreeUnicodeString(&pdx->interfaceName);

	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	//删除fdo
	IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave HandleRemoveDevice\n"));
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

NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP Irp, PKEVENT pev)
{							
	// OnRequestComplete
	//在完成例程中设置等待事件
	KeSetEvent(pev, 0, FALSE);
	//标志本IRP还需要再次被完成
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP Irp)
{							
	KEVENT event;
	//初始化事件
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	//将本层堆栈拷贝到下一层堆栈
	IoCopyCurrentIrpStackLocationToNext(Irp);
	//设置完成例程
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)OnRequestComplete,
		(PVOID) &event, TRUE, TRUE, TRUE);

	//调用底层驱动，即PDO
	IoCallDriver(pdx->NextStackDevice, Irp);
	//等待PDO完成
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	return Irp->IoStatus.Status;
}

NTSTATUS CompleteIrp(IN PIRP Irp, IN NTSTATUS status, IN ULONG info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

#define SetMostPoweredState(SystemState, OurDeviceState) \
	dps = deviceCapabilities->DeviceState[SystemState]; \
	if(dps == PowerDeviceUnspecified || dps>OurDeviceState) \
		deviceCapabilities->DeviceState[SystemState] = OurDeviceState;

NTSTATUS PnpQueryCapabilitiesHandler(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	NTSTATUS status = ForwardAndWait(pdx, pIrp);
	if (NT_SUCCESS(status))
	{
		PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(pIrp);
		PDEVICE_CAPABILITIES deviceCapabilities;
		deviceCapabilities = IrpStack->Parameters.DeviceCapabilities.Capabilities;

		for (int ds = PowerSystemWorking; ds < PowerSystemMaximum; ds++)
			KdPrint(("Capabilities from bus: DeviceState[%d]=%d\n", ds, deviceCapabilities->DeviceState[ds]));
		DEVICE_POWER_STATE dps;

		SetMostPoweredState(PowerSystemWorking, PowerDeviceD0);
		SetMostPoweredState(PowerSystemSleeping1, PowerDeviceD3);
		SetMostPoweredState(PowerSystemSleeping2, PowerDeviceD3);
		SetMostPoweredState(PowerSystemSleeping3, PowerDeviceD3);
		SetMostPoweredState(PowerSystemHibernate, PowerDeviceD3);
		SetMostPoweredState(PowerSystemShutdown, PowerDeviceD3);

		deviceCapabilities->Removable = TRUE;
		for (int ds = PowerSystemWorking; ds < PowerSystemMaximum; ds++)
		{
			KdPrint(("Capabilities now: DeviceState[%d]=%d", ds, deviceCapabilities->DeviceState[ds]));
		}
	}
	return CompleteIrp(pIrp, status, pIrp->IoStatus.Information);
}

NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	KdPrint(("Enter HelloWDMPnp\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP Irp) =
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
		PnpQueryCapabilitiesHandler,		// IRP_MN_QUERY_CAPABILITIES
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
		status = DefaultPnpHandler(pdx, Irp); // some function we don't know about
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

	status = (*fcntab[fcn])(pdx, Irp);
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

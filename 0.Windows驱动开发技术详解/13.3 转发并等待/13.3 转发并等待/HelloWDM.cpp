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

NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	KdPrint(("Enter HelloWDMAddDevice\n"));

	NTSTATUS status;
	PDEVICE_OBJECT fdo;
	status = IoCreateDevice(
		pDriverObject,
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

NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	KdPrint(("Enter DefaultPnpHandler\n"));
	IoSkipCurrentIrpStackLocation(Irp);
	KdPrint(("Leave DefaultPnpHandler\n"));
	return IoCallDriver(pdx->NextStackDevice, Irp);
}

NTSTATUS OnRequestComplete(PDEVICE_OBJECT junk, PIRP pIrp, PKEVENT pev)
{
	KeSetEvent(pev, 0, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS ForwardAndWait(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	//将本层堆栈拷贝到下一层堆栈
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	//设置完成例程
	IoSetCompletionRoutine(pIrp, (PIO_COMPLETION_ROUTINE)OnRequestComplete, (PVOID)&event, TRUE, TRUE, TRUE);

	//调用底层驱动，即PDO
	IoCallDriver(pdx->NextStackDevice, pIrp);

	//等待PDO完成
	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
	return pIrp->IoStatus.Status;
}

NTSTATUS HandleRemoveDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	KdPrint(("Enter HandleRemoveDevice\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, pIrp);

	IoSetDeviceInterfaceState(&pdx->interfaceName, FALSE);
	RtlFreeUnicodeString(&pdx->interfaceName);

	//调用IoDetachDevice()把fdo从设备栈中脱开：
	if (pdx->NextStackDevice)
		IoDetachDevice(pdx->NextStackDevice);

	//删除fdo：
	IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave HandleRemoveDevice\n"));
	return status;
}

VOID ShowResources(PCM_PARTIAL_RESOURCE_LIST list)
{
	//枚举资源
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG nres = list->Count;
	ULONG i;

	for (i = 0; i < nres; ++i, ++resource)
	{						// for each resource
		ULONG type = resource->Type;

		static char* name[] = {
			"CmResourceTypeNull",
			"CmResourceTypePort",
			"CmResourceTypeInterrupt",
			"CmResourceTypeMemory",
			"CmResourceTypeDma",
			"CmResourceTypeDeviceSpecific",
			"CmResourceTypeBusNumber",
			"CmResourceTypeDevicePrivate",
			"CmResourceTypeAssignedResource",
			"CmResourceTypeSubAllocateFrom",
		};

		KdPrint(("    type %s", type < arraysize(name) ? name[type] : "unknown"));

		switch (type)
		{					// select on resource type
		case CmResourceTypePort:
		case CmResourceTypeMemory:
			KdPrint((" start %8X%8.8lX length %X\n",
				resource->u.Port.Start.HighPart, resource->u.Port.Start.LowPart,
				resource->u.Port.Length));
			break;

		case CmResourceTypeInterrupt:
			KdPrint(("  level %X, vector %X, affinity %X\n",
				resource->u.Interrupt.Level, resource->u.Interrupt.Vector,
				resource->u.Interrupt.Affinity));
			break;

		case CmResourceTypeDma:
			KdPrint(("  channel %d, port %X\n",
				resource->u.Dma.Channel, resource->u.Dma.Port));
		}					// select on resource type
	}
}

NTSTATUS HandleStartDevice(PDEVICE_EXTENSION pdx, PIRP pIrp)
{
	KdPrint(("Enter HandleStartDevice\n"));

	//转发IRP并等待返回
	NTSTATUS status = ForwardAndWait(pdx, pIrp);
	if (!NT_SUCCESS(status))
	{
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}

	//得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	//从当前堆栈得到源信息
	PCM_PARTIAL_RESOURCE_LIST raw;
	if (stack->Parameters.StartDevice.AllocatedResources)
		raw = &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
	else
		raw = NULL;

	KdPrint(("Show raw resouce\n"));
	ShowResources(raw);
	
	//从当前堆栈得到翻译信息
	PCM_PARTIAL_RESOURCE_LIST translated;
	if (stack->Parameters.StartDevice.AllocatedResourcesTranslated)
		translated = &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
	else
		translated = NULL;

	KdPrint(("Show translated resouces\n"));
	ShowResources(translated);

	//完成IRP
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HandleStartDevice\n"));
	return status;
}

NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT fdo, IN PIRP pIrp)
{
	KdPrint(("Enter HelloWDMPnp\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
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

NTSTATUS HelloWDMDispatchRoutine(IN PDEVICE_OBJECT fdo, IN PIRP pIrp)
{
	KdPrint(("Enter HelloWDMDispatchRoutine\n"));
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloWDMDispatchRoutine\n"));
	return STATUS_SUCCESS;
}

void HelloWDMUnload(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Enter HelloWDMUnload\n"));
	KdPrint(("Leave HelloWDMUnlaod\n"));
}



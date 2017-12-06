#include "stddcls.h"
#include "Driver.h"

#include <srb.h>
#include <scsi.h>

NTSTATUS AddDevice(PDRIVER_OBJECT pDriverObject, PDEVICE_OBJECT pdo);
VOID DriverUnload(PDRIVER_OBJECT fido);
NTSTATUS DispatchAny(PDEVICE_OBJECT fido, PIRP pIrp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fido, PIRP pIrp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fido, PIRP pIrp);
NTSTATUS DispatchWmi(PDEVICE_OBJECT fido, PIRP pIrp);
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo);
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP pIrp, PDEVICE_EXTENSION pdx);
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP pIrp, PDEVICE_EXTENSION pdx);

/***************************************************************************************************/

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING RegistryPath)
{
	KdPrint((DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", pDriverObject));

	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->DriverExtension->AddDevice = AddDevice;
	for (int i = 0; i < arraysize(pDriverObject->MajorFunction); ++i)
	{
		pDriverObject->MajorFunction[i] = DispatchAny;
	}

	pDriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	pDriverObject->MajorFunction[IRP_MJ_SCSI] = DispatchForSCSI;

	return STATUS_SUCCESS;
}


VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	KdPrint((DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", pDriverObject));
}

NTSTATUS AddDevice(PDRIVER_OBJECT pDriverObject, PDEVICE_OBJECT pdo)
{
	NTSTATUS status;

	PDEVICE_OBJECT fido;
	status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), NULL, GetDeviceTypeToUse(pdo), 0, FALSE, &fido);

	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVERNAME "- IoCreateDevice failed - %X\n", status));
		return status;
	}
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;

	do
	{
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);
		pdx->pDeviceObject = fido;
		pdx->pdo = pdo;

		//将过滤驱动附加在底层驱动之上
		PDEVICE_OBJECT fdo = IoAttachDeviceToDeviceStack(fido, pdo);
		if (!fdo)
		{
			KdPrint((DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
		}
		//记录底层驱动
		pdx->pLowerDeviceObject = fdo;
		//由于不知道底层驱动是直接IO还是BufferIO,因此将标志都置上
		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);

		fido->Flags &= ~DO_DEVICE_INITIALIZING;
	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		if (pdx->pLowerDeviceObject)
			IoDetachDevice(pdx->pLowerDeviceObject);
		IoDeleteDevice(fido);
	}

	return status;
}

NTSTATUS CompleteRequest(PIRP pIrp, NTSTATUS status, ULONG_PTR info)
{
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
USBSCSICompletion(PDEVICE_OBJECT pDeviceObject,PIRP pIrp,PVOID Context)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	IoAcquireRemoveLock(&pdx->RemoveLock, pIrp);

	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);

	PSCSI_REQUEST_BLOCK CurSrb = irpStack->Parameters.Scsi.Srb;
	PCDB cdb = (PCDB)CurSrb->Cdb;
	UCHAR opCode = cdb->CDB6GENERIC.OperationCode;

	if (opCode == SCSIOP_MODE_SENSE && CurSrb->DataBuffer && CurSrb->DataTransferLength >= sizeof(MODE_PARAMETER_HEADER))
	{
		KdPrint(("SCSIOP_MODE_SENSE comming!\n"));

		PMODE_PARAMETER_HEADER modeData = (PMODE_PARAMETER_HEADER)CurSrb->DataBuffer;

		modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	}

	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}

	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);

	return pIrp->IoStatus.Status;
}

NTSTATUS DispatchForSCSI(PDEVICE_OBJECT fido, PIRP pIrp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;

	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(pIrp);

	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, pIrp);
	if (!NT_SUCCESS(status))
	{
		return CompleteRequest(pIrp, status, 0);
	}

	IoCopyCurrentIrpStackLocationToNext(pIrp);

	IoSetCompletionRoutine(pIrp,
		USBSCSICompletion,
		NULL,
		TRUE,
		TRUE,
		TRUE);
	status = IoCallDriver(pdx->pLowerDeviceObject, pIrp);
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	return status;
}

NTSTATUS DispatchAny(PDEVICE_OBJECT fido, PIRP pIrp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

#if DBG
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
#endif

	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, pIrp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(pIrp, status, 0);
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pdx->pLowerDeviceObject, pIrp);
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	return status;
}

NTSTATUS DispatchPower(PDEVICE_OBJECT fido, PIRP pIrp)
{
#if DBG
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG fcn = stack->MinorFunction;
	static char* fcnname[] = 
	{
		"IRP_MN_WAIT_WAKE",
		"IRP_MN_POWER_SEQUENCE",
		"IRP_MN_SET_POWER",
		"IRP_MN_QUERY_POWER",
	};

	if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
	{
		static char* sysstate[] =
		{
			"PowerSystemUnspecified",
			"PowerSystemWorking",
			"PowerSystemSleeping1",
			"PowerSystemSleeping2",
			"PowerSystemSleeping3",
			"PowerSystemHibernate",
			"PowerSystemShutdown",
			"PowerSystemMaximum",
		};

		static char* devstate[] =
		{
			"PowerDeviceUnspecified",
			"PowerDeviceD0",
			"PowerDeviceD1",
			"PowerDeviceD2",
			"PowerDeviceD3",
			"PowerDeviceMaximum",
		};

		ULONG context = stack->Parameters.Power.SystemContext;
		POWER_STATE_TYPE type = stack->Parameters.Power.Type;
		KdPrint((DRIVERNAME " - IRP_MJ_POWER(%s)", fcnname[fcn]));
		if (type == SystemPowerState)
			KdPrint((", SystemPowerState = %s\n", sysstate[stack->Parameters.Power.State.SystemState]));
		else
			KdPrint((", DevicePowerState = %s\n", devstate[stack->Parameters.Power.State.DeviceState]));
	}
	else if (fcn < arraysize(fcnname))
		KdPrint((DRIVERNAME " - IRP_MJ_POWER (%s)\n", fcnname[fcn]));
	else
		KdPrint((DRIVERNAME " - IRP_MJ_POWER (%2.2X)\n", fcn));
#endif

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	PoStartNextPowerIrp(pIrp);
	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, pIrp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(pIrp, status, 0);
	IoSkipCurrentIrpStackLocation(pIrp);
	status = PoCallDriver(pdx->pLowerDeviceObject, pIrp);
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	return status;
}

NTSTATUS DispatchPnp(PDEVICE_OBJECT fido, PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG fcn = stack->MinorFunction;
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, pIrp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(pIrp, status, 0);
#if DBG
	static char* pnpname[] = 
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
		"IRP_MN_QUERY_LEGACY_BUS_INFORMATION",
	};

	if (fcn < arraysize(pnpname))
		KdPrint((DRIVERNAME " - IRP_MJ_PNP (%s)\n", pnpname[fcn]));
	else
		KdPrint((DRIVERNAME " - IRP_MJ_PNP (%2.2X)\n", fcn));
#endif

	if (fcn == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		if (!fido->AttachedDevice || (fido->AttachedDevice->Flags & DO_POWER_PAGABLE))
			fido->Flags |= DO_POWER_PAGABLE;
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(pIrp, (PIO_COMPLETION_ROUTINE)UsageNotificationCompletionRoutine, (PVOID)pdx, TRUE, TRUE, TRUE);

		return IoCallDriver(pdx->pLowerDeviceObject, pIrp);
	}

	if (fcn == IRP_MN_START_DEVICE)
	{
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(pIrp, (PIO_COMPLETION_ROUTINE)StartDeviceCompletionRoutine,
			(PVOID)pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->pLowerDeviceObject, pIrp);
	}

	if (fcn == IRP_MN_REMOVE_DEVICE)
	{
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(pdx->pLowerDeviceObject, pIrp);
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, pIrp);
		RemoveDevice(fido);

		return status;
	}

	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pdx->pLowerDeviceObject, pIrp);
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);

	return status;
}

ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT ldo = IoGetAttachedDeviceReference(pdo);
	if (!ldo)
		return FILE_DEVICE_UNKNOWN;
	ULONG devtype = ldo->DeviceType;
	ObDereferenceObject(ldo);
	return devtype;
}

VOID RemoveDevice(PDEVICE_OBJECT fido)
{
	KdPrint(("Enter RemoveDevice\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	if (pdx->pLowerDeviceObject)
		IoDetachDevice(pdx->pLowerDeviceObject);
	IoDeleteDevice(fido);
}

NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP pIrp, PDEVICE_EXTENSION pdx)
{
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	if (pdx->pLowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		fido->Characteristics |= FILE_REMOVABLE_MEDIA;
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	return STATUS_SUCCESS;
}

NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP pIrp, PDEVICE_EXTENSION pdx)
{
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);
	if (!(pdx->pLowerDeviceObject->Flags & DO_POWER_PAGABLE))
		fido->Flags &= ~DO_POWER_PAGABLE;
	IoReleaseRemoveLock(&pdx->RemoveLock, pIrp);
	return STATUS_SUCCESS;
}

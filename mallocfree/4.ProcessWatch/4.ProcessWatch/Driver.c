#include "Driver.h"


#define DeviceName	L"\\Device\\HelloDev"
#define LinkName	L"\\??\\HelloDev"

#define	EVENT_NAME	L"\\BaseNamedObjects\\ProcWatch"
PDEVICE_OBJECT g_pDeviceObject;
PKEVENT			g_pEvent = NULL;

#define IOCTRL_BASE		0x800

#define MyCode(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTRL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)
	
#define RECV_INFO	MyCode(0)
#define RECV_EVENT	MyCode(1)

typedef struct _Info {
	HANDLE hProcessId;
	BOOLEAN Create;
}INFO, *PINFO;

NTSTATUS CommonDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	
	PVOID buff = pIrp->AssociatedIrp.SystemBuffer;
	HANDLE hEvent = *(HANDLE*)buff;

	NTSTATUS status;
	
	

	
	ULONG uInputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG uOutputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (code)
	{
	case RECV_EVENT:
	{
		status = ObReferenceObjectByHandle(hEvent,
			EVENT_MODIFY_STATE,
			*ExEventObjectType,
			KernelMode,
			(PVOID*)&g_pEvent,
			NULL);
		if (NT_SUCCESS(status))
		{
			KdPrint(("Kernel:%X \n", g_pEvent));

		}
		else
		{
			KdPrint(("ObRef Error: %X\n", status));
		}

		pIrp->IoStatus.Information = sizeof(HANDLE);
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		break;
	}

	case RECV_INFO:
	{
		PINFO info = (PINFO)buff;

		info->hProcessId = pDevExt->ProcessId;
		info->Create = pDevExt->Create;

		pIrp->IoStatus.Information = sizeof(INFO);
		pIrp->IoStatus.Status = STATUS_SUCCESS;

		if(g_pEvent)
			KeClearEvent(g_pEvent);
	}
		break;
	default:
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
	}
	break;
	}

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING ustrSymbolic;
	UNICODE_STRING ustrDevName;

	PsSetCreateProcessNotifyRoutine(SetCreateProcessNotifyRoutine, TRUE);


	RtlInitUnicodeString(&ustrSymbolic, LinkName);
	RtlInitUnicodeString(&ustrDevName, DeviceName);

	IoDeleteSymbolicLink(&ustrSymbolic);

	IoDeleteDevice(pDriverObject->DeviceObject);


	if (g_pEvent)
	{
		ObDereferenceObject(g_pEvent);
	}

}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING ustrDevName;
	UNICODE_STRING ustrLinkName;
	NTSTATUS status;
	UNICODE_STRING ustrEventName;
	HANDLE hEvent;
	//PDEVICE_OBJECT pDevObj;

	RtlInitUnicodeString(&ustrDevName, DeviceName);
	RtlInitUnicodeString(&ustrLinkName, LinkName);

	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&g_pDeviceObject);
	
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	g_pDeviceObject->Flags |= DO_BUFFERED_IO;

	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(g_pDeviceObject);
		return status;
	}

	RtlInitUnicodeString(&ustrEventName, EVENT_NAME);
	/*g_pEvent = IoCreateNotificationEvent(&ustrEventName, &g_hEvent);
	KeClearEvent(g_pEvent);*/
	
	return status;
}



void SetCreateProcessNotifyRoutine(HANDLE  ParentId,HANDLE  ProcessId,BOOLEAN Create)
{
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)g_pDeviceObject->DeviceExtension;

	pDevExt->Create = Create;
	pDevExt->ParentId = ParentId;
	pDevExt->ProcessId = ProcessId;

	if(g_pEvent != NULL)
		KeSetEvent(g_pEvent, 0, FALSE);

}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegString)
{
	NTSTATUS status;
	pDriverObject->DriverUnload = DriverUnload;
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = CommonDispatch;
	}

	status = CreateDevice(pDriverObject);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;

	PsSetCreateProcessNotifyRoutine(SetCreateProcessNotifyRoutine, FALSE);

	KdPrint(("Enter DriverEntry\n"));

	return status;
}

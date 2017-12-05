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
		DbPrint(("Keyboard Create Device Failed..\n"));

		RtlFreeUnicodeString(&ntUnicodeString);

		return STATUS_SUCCESS;
	}

}




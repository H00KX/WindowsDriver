#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif


#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	UNICODE_STRING ustrDeviceName;
	UNICODE_STRING ustrSymLinkName;

	KDPC pollingDPC;
	KTIMER pollingTimer;
	PIRP currentPendingIrp;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// º¯ÊýÉùÃ÷

NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
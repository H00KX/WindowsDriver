#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif

#define PAGEDCODE	code_seg("PAGE")
#define LOCKEDCODE	code_seg()
#define INITCODE	code_seg("INIT")
#define PAGEDDATA	data_seg("PAGE")
#define LOCKEDDATA	data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p)	(sizeof(p)/sizeof((p)[0]))

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT	pDevice;
	UNICODE_STRING	ustrDeviceName;	//Éè±¸Ãû³Æ
	UNICODE_STRING	ustrSymLinkName;

	KDPC	pollingDPC;
	KTIMER	pollingTimer;
	PIRP	currentPendingIRP;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);

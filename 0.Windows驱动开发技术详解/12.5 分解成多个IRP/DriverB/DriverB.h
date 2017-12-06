#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include<ntddk.h>
#ifdef __cplusplus
}
#endif

#define PAGEDCODE	code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p)	(sizeof(p)/sizeof((p)[0]))

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT	pDevice;
	UNICODE_STRING ustrDeviceName;
	PDEVICE_OBJECT TargetDevice;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _MyDriver_RW_CONTEXT
{
	PMDL				NewMdl;		//ÐÂMDL
	PMDL				PreviousMdl;	//¾ÉMDL
	ULONG				Length;
	ULONG				Numxfer;
	ULONG_PTR			VirtualAddress;
	PDEVICE_EXTENSION	DeviceExtension;
}MYDRIVER_RW_CONTEXT, *PMYDRIVER_RW_CONTEXT;


NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
NTSTATUS HelloDDKRead(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);
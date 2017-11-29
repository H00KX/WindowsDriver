#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <ntddk.h>

#ifdef __cplusplus
}
#endif

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p)	(sizeof(p) / sizeof((p)[0]))

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT	pDevice;
	UNICODE_STRING	ustrDeviceName;
	UNICODE_STRING	ustrSymLinkName;

	KDPC	pollingDPC;			//存储DPC对象
	KTIMER	pollingTimer;		//存储计时器对象
	PIRP	currentPendingIRP;	//记录当前挂起的IRP
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//函数声明
NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS HelloDDKCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS HelloDDKClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);


#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE	code_seg("INIT")

#define PAGEDDATA	data_seg("PAGE")
#define LOCKEDDATA	data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define TIMER_OUT	3

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT	pDevice;
	UNICODE_STRING	ustrDeviceName;		//设备名称
	UNICODE_STRING	ustrSymLinkName;	//符号链接名

	KDPC	pollingDPC;					//存储DPC对象
	KTIMER	pollingTimer;				//存储时钟对象
	LARGE_INTEGER	pollingInterval;	//记录计时器间隔时间
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//函数声明
NTSTATUS	CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID		HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS	HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp);

NTSTATUS HelloDDKDeviceIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp);
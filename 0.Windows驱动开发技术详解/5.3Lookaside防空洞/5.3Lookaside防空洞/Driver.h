

#ifdef __cplusplus
extern "C" {
#endif

#include <ntddk.h>
#include <ntddk.h>
#ifdef __cplusplus
}
#endif

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	UNICODE_STRING ustrDeviceName;		//设备名称
	UNICODE_STRING ustrSymLinkName;		//符号链接名
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//函数声明
NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
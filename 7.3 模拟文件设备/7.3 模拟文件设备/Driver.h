
#ifdef __cplusplus
extern "C" {
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif


typedef struct _DEVICE_EXTENSION {
	ULONG uMaxLength;
	ULONG uLength;
	PCHAR pBuffer;
	PDEVICE_OBJECT pDeviceObj;
	UNICODE_STRING usymLinName;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;


NTSTATUS CreateDevice(PDRIVER_OBJECT pDriver_Object);
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject);

NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevice_Object, PIRP pIrp);
NTSTATUS CloseDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS CleanDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);



NTSTATUS ReadDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS WriteDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS QueryInfoDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);


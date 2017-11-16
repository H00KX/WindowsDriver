

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
	UNICODE_STRING ustrDeviceName;		//�豸����
	UNICODE_STRING ustrSymLinkName;		//����������
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//��������
NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject);
VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp);
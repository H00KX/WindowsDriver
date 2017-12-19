#include <ntddk.h>

typedef struct _DeviceExtension
{
	UNICODE_STRING ustrDevName;
	HANDLE	ParentId;
	HANDLE	ProcessId;
	BOOLEAN Create;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

VOID DriverUnload(PDRIVER_OBJECT pDriverObject);

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegString);

NTSTATUS CommonDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp);

NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDevObj, PIRP pIrp);

void SetCreateProcessNotifyRoutine(
	_In_ HANDLE  ParentId,
	_In_ HANDLE  ProcessId,
	_In_ BOOLEAN Create);
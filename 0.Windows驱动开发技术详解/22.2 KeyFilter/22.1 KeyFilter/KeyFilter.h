

#define KEY_UP			1	
#define KEY_DOWN		0

#define LCONTROL	((USHORT)0x1D)
#define CAPS_LOCK	((USHORT)0x3A)

#if DBG
#define DbgPrint(arg)	DbgPrint arg
#else
#define DbgPrint(arg)
#endif // DGB


NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(PUNICODE_STRING String);

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT TopOfStack;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING ustrRegistryPath);

NTSTATUS Ctrl2capAddDevice(PDRIVER_OBJECT pDriver, PDEVICE_OBJECT pdo);

NTSTATUS Ctrl2capPnp(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS Ctrl2capPower(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

VOID Ctrl2capUnload(PDRIVER_OBJECT pDriver);

NTSTATUS Ctrl2capInit(PDRIVER_OBJECT pDriverObject);

NTSTATUS Ctrl2capDispatchRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS Ctrl2capDispatchGeneral(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

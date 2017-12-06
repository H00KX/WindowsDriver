#ifndef DRIVER_H
#define DRIVER_H

#define DRIVERNAME	"MyFileFilter(BX)"

typedef struct tagDEVICE_EXTENSION {
	PDEVICE_OBJECT pDeviceObject;
	PDEVICE_OBJECT pLowerDeviceObject;
	PDEVICE_OBJECT pdo;		//THE PDO
	IO_REMOVE_LOCK RemoveLock;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


VOID RemoveDevice(PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(PIRP pIrp, NTSTATUS status, ULONG_PTR info);
NTSTATUS DispatchForSCSI(PDEVICE_OBJECT fido, PIRP pIrp);

#endif
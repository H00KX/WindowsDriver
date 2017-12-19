#ifndef _COMCAP_HEADER_
#define _COMCAP_HEADER_

void cppSetEnable(BOOLEAN enable);

//过滤所有的irp
BOOLEAN ccpIrpFilter(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, NTSTATUS *status);

BOOLEAN ccpFileIrpFilter(PDEVICE_OBJECT next_dev, PIRP pIrp, PIO_STACK_LOCATION irpsp, NTSTATUS *status);

//卸载时调用，可以解除绑定
void ccpUnload();

//绑定所有串口，在DriverEntry中调用
void ccpAttachAllComs(PDRIVER_OBJECT pDriverObject);

enum {
	CCP_IRP_PASS = 0,
	CCP_IRP_COMPLETED = 1,
	CCP_IRP_GO_ON = 2
};

extern int ccpIrpPreCallback(
	PDEVICE_OBJECT device,
	PDEVICE_OBJECT next_dev,
	PIRP irp,
	ULONG i,
	PVOID *context
);

extern void ccpIrpPosCallback(
	PDEVICE_OBJECT device,
	PDEVICE_OBJECT next_dev,
	PIRP irp,
	PIO_STACK_LOCATION irpsp,
	ULONG i,
	PVOID context
);

#endif
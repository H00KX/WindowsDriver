#include <ntddk.h>
#define CLD_BASE	0x800
#define MyCode(i) \
	CTL_CODE(FILE_DEVICE_UNKNOWN, CLD_BASE, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DelFile	MyCode(0)

NTSTATUS DispatchCommon(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}


NTSTATUS DispatchIoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status;
	ULONG code;
	PIO_STACK_LOCATION stack;
	ULONG uInput;
	ULONG uOutput;
	PVOID Input;
	PVOID Output;

	stack = IoGetCurrentIrpStackLocation(pIrp);

	uInput = stack->Parameters.DeviceIoControl.InputBufferLength;
	uOutput = stack->Parameters.DeviceIoControl.OutputBufferLength;
	code = stack->Parameters.DeviceIoControl.IoControlCode;

	Input = Output = pIrp->AssociatedIrp.SystemBuffer;

	switch (code)
	{
	case DelFile:
		break;

	default:
		break;
	}


}


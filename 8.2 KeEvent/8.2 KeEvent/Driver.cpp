#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

#define IOCTL_TRANSMIT_EVENT CTL_CODE(FILE_DEVICE_UNKNOWN, \
	0x801, \
	METHOD_BUFFERED, \
	FILE_ANY_ACCESS)

/*****************************************************************************************/
//内核模式下的 event
VOID MyProcessThread(IN PVOID pContext)
{
	PKEVENT pEvent = (PKEVENT)pContext;
	KdPrint(("Enter MyProcessThread\n"));

	//设置事件
	KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);

	KdPrint(("Leave MyProcessThread\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}
VOID Test()
{
	HANDLE hMyThread;
	KEVENT kEvent;

	//初始化内核对象
	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	NTSTATUS status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread, &kEvent);

	KeWaitForSingleObject(&kEvent, Executive, KernelMode, FALSE, NULL);
}

/******************************************************************************************/
//交互模式下的 event

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//注册其他驱动调用函数入口
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	KdPrint(("DriverEntry end\n"));
	return status;
}


NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT pDriverObject
)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;

	RtlInitUnicodeString(&devName, DevName);

	//创建设备
	status = IoCreateDevice(
		pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;
	//创建符号链接
	UNICODE_STRING symLinkName;

	RtlInitUnicodeString(&symLinkName, SymLinkName);
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}


VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)                         //遍历设备对象，并删除
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//删除符号链接
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

NTSTATUS HelloDDKDeviceIOControl(IN PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("Enter HelloDDKDeviceIoControl\n"));
	//获得当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	//获得输入参数和输出参数大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//得到IOCTL码
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG info = 0;
	switch (code)
	{
	case IOCTL_TRANSMIT_EVENT:
	{
		KdPrint(("IOCTL_TEST1\n"));
		//得到应用程序传递进来的事件
		HANDLE hUserEvent = *(HANDLE*)pIrp->AssociatedIrp.SystemBuffer;
		PKEVENT pEvent;
		status = ObReferenceObjectByHandle(hUserEvent,EVENT_MODIFY_STATE,*ExEventObjectType,
			KernelMode,
			(PVOID*)&pEvent,
			NULL);
		//设置事件
		KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
		//减小引用计数
		ObDereferenceObject(pEvent);

		break;
	}
	default:
		status = STATUS_INVALID_VARIANT;
	}
	//设置IRP完成状态
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	//结束IRP请求
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDeviceIOControl\n"));
	return status;
}

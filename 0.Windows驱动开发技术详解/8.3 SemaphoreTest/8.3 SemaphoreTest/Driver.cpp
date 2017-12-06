#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"

VOID MyProcessThread(PVOID pContext)
{
	PKSEMAPHORE pkSemaphore = (PKSEMAPHORE)pContext;
	KdPrint(("Enter MyProcessThread\n"));
	KeReleaseSemaphore(pkSemaphore, IO_NO_INCREMENT, 1, FALSE);
	KdPrint(("Leave MyProcessThread\n"));
	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID Test()
{
	HANDLE hMyThread;
	KSEMAPHORE kSemaphore;
	//初始化内核信号量
	KeInitializeSemaphore(&kSemaphore, 2, 2);
	//读取信号量状态
	LONG count = KeReadStateSemaphore(&kSemaphore);
	KdPrint(("The semaphore count is %s\n", count));
	//等待信号量
	KeWaitForSingleObject(&kSemaphore, Executive, KernelMode, FALSE, NULL);

	//读取
	count = KeReadStateSemaphore(&kSemaphore);
	KdPrint(("The Semaphore count is %d\n", count));
	//创建线程
	NTSTATUS status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread, &kSemaphore);

	KeWaitForSingleObject(&kSemaphore, Executive, KernelMode, FALSE, NULL);
	KdPrint(("After KeWaitForSingleObject\n"));
}

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
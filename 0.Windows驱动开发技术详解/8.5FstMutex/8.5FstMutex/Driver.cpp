#include "Driver.h"
#define DevName L"\\Device\\MyDDKDevice"
#define SymLinkName L"\\??\\HelloDDK"
//快速互斥体不能递归获取

VOID MyProcessThread1(PVOID pContext)
{
	PFAST_MUTEX pFastMutex = (PFAST_MUTEX)pContext;
	//获得快速互斥体
	ExAcquireFastMutex(pFastMutex);
	KdPrint(("Enter MyProcessThread1\n"));
	//强迫停止50ms,模拟执行一段代码
	KeStallExecutionProcessor(50);

	KdPrint(("Leave MyProcessThread1\n"));


	//释放快速互斥体
	ExReleaseFastMutex(pFastMutex);
	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID MyProcessThread2(IN PVOID pContext)
{
	PFAST_MUTEX pFastMutex = (PFAST_MUTEX)pContext;
	//获得快速结构体
	ExAcquireFastMutex(pFastMutex);
	KdPrint(("Enter MyProcessThread2\n"));
	KeStallExecutionProcessor(50);

	KdPrint(("Leave MyProcessThread2\n"));

	ExReleaseFastMutex(pFastMutex);
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID Test()
{
	HANDLE hMyThread1, hMyThread2;
	FAST_MUTEX fastMutex;

	ExInitializeFastMutex(&fastMutex);

	PsCreateSystemThread(&hMyThread1, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread1, &fastMutex);

	PsCreateSystemThread(&hMyThread2, 0, NULL, NtCurrentProcess(), NULL, MyProcessThread2, &fastMutex);

	PVOID Pointer_Array[2];
	ObReferenceObjectByHandle(hMyThread1, 0, NULL, KernelMode, &Pointer_Array[0], NULL);
	ObReferenceObjectByHandle(hMyThread2, 0, NULL, KernelMode, &Pointer_Array[2], NULL);

	KeWaitForMultipleObjects(2, Pointer_Array, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);


	ObDereferenceObject(Pointer_Array[0]);
	ObDereferenceObject(Pointer_Array[1]);

	KdPrint(("After KeWaitForMultipleObject\n"));
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

	Test();

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
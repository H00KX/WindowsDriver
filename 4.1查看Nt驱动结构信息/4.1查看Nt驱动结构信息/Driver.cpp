#include "Driver.h"

#pragma INITCODE
void Dump(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("-------------------------------------------------------------\n"));
	KdPrint(("Begin Dump...\n"));
	KdPrint(("Driver Address:0X%08X\n", pDriverObject));
	KdPrint(("Driver name:%wZ\n", &pDriverObject->DriverName));
	KdPrint(("Driver HardwareDatabase:%wZ\n", pDriverObject->HardwareDatabase));
	KdPrint(("Driver first device:0X%08X\n", pDriverObject->DeviceObject));

	PDEVICE_OBJECT	pDevice = pDriverObject->DeviceObject;
	int i = 1;
	for (; pDevice != NULL; pDevice = pDevice->NextDevice)
	{
		KdPrint(("The %d device\n", i++));
		KdPrint(("Device AttachedDevice: 0X%08X\n", pDevice->AttachedDevice));
		KdPrint(("Device NextDevice: 0X%08X\n", pDevice->NextDevice));
		KdPrint(("Device StackSize: %d\n", pDevice->StackSize));
		KdPrint(("Device's DriverObject: 0x%08X\n", pDevice->DriverObject));
	}
	KdPrint(("Dump over!\n"));
	KdPrint(("--------------------------------------------------------------------\n"));
}

#pragma PAGEDCODE
NTSTATUS  CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS  status;
	PDEVICE_OBJECT	pDevObj;
	PDEVICE_EXTENSION	pDevExt;

	//创建设备名称
	UNICODE_STRING	devName;
	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

	//创建设备
	status = IoCreateDevice(
		pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj
	);

	if (!NT_SUCCESS(status))
		return status;
	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	//pDevExt->ustrDeviceName = devName;

	pDevExt->ustrDeviceName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, 260 * sizeof(WCHAR), 'POC');
	RtlZeroMemory(pDevExt->ustrDeviceName.Buffer, 260 * sizeof(WCHAR));
	RtlCopyMemory(pDevExt->ustrDeviceName.Buffer, devName.Buffer, devName.Length);
	pDevExt->ustrDeviceName.Length = devName.Length;
	pDevExt->ustrDeviceName.MaximumLength = 260 * sizeof(WCHAR);

	

	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK");
	//pDevExt->ustrSymLinkName = symLinkName;
	

	pDevExt->ustrSymLinkName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, 260 * sizeof(WCHAR), 'POC');
	RtlZeroMemory(pDevExt->ustrSymLinkName.Buffer, 260 * sizeof(WCHAR));
	RtlCopyMemory(pDevExt->ustrSymLinkName.Buffer, symLinkName.Buffer, symLinkName.Length);
	pDevExt->ustrSymLinkName.Length = symLinkName.Length;
	pDevExt->ustrSymLinkName.MaximumLength = 260 * sizeof(WCHAR);

	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS CreateDevice2(
	IN PDRIVER_OBJECT	pDriverObject
)
{
	NTSTATUS	status;
	PDEVICE_OBJECT	pDevObj;
	PDEVICE_EXTENSION	pDevExt;

	//创建设备名称
	UNICODE_STRING	devName;
	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice2");

	//创建设备
	status = IoCreateDevice(
		pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj
	);

	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	//pDevExt->ustrDeviceName = devName;
	

	pDevExt->ustrDeviceName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, 260 * sizeof(WCHAR), 'POC');
	RtlZeroMemory(pDevExt->ustrDeviceName.Buffer, 260 * sizeof(WCHAR));
	RtlCopyMemory(pDevExt->ustrDeviceName.Buffer, devName.Buffer, devName.Length);
	pDevExt->ustrDeviceName.Length = devName.Length;
	pDevExt->ustrDeviceName.MaximumLength = 260 * sizeof(WCHAR);



	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK2");

	//pDevExt->ustrSymLinkName = symLinkName;
	pDevExt->ustrSymLinkName.Buffer = (PWCH)ExAllocatePoolWithTag(PagedPool, 260 * sizeof(WCHAR), 'POC');
	RtlZeroMemory(pDevExt->ustrSymLinkName.Buffer, 260 * sizeof(WCHAR));
	RtlCopyMemory(pDevExt->ustrSymLinkName.Buffer, symLinkName.Buffer, symLinkName.Length);
	pDevExt->ustrSymLinkName.Length = symLinkName.Length;
	pDevExt->ustrSymLinkName.MaximumLength = 260 * sizeof(WCHAR);



	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}

#pragma INITCODE
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	//PDEVICE_OBJECT	pNextObj;
	//KdPrint(("Enter DriverUnload\n"));
	//pNextObj = pDriverObject->DeviceObject;
	//while (pNextObj != NULL)
	//{
	//	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

	//	//删除符号链接
	//	UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
	//	IoDeleteSymbolicLink(&pLinkName);
	//	pNextObj = pNextObj->NextDevice;
	//	IoDeleteDevice(pDevExt->pDevice);

	//	ExFreePool(pDevExt->ustrDeviceName.Buffer);
	//	ExFreePool(pDevExt->ustrSymLinkName.Buffer);
	//}
}

#pragma PAGEDCODE
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));

	return status;
}

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//注册其他驱动调用函数入口
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	//创建驱动对象
	status = CreateDevice(pDriverObject);
	status = CreateDevice2(pDriverObject);

	Dump(pDriverObject);

	KdPrint(("DriverEntry end\n"));

	return status;

}



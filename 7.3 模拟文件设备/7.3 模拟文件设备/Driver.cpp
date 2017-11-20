#include "Driver.h"

#define devName L"\\Device\\MyDDKTest"
#define symLinkName L"\\??\\MyHelloDDK"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg)
{
	KdPrint(("Enter DriverEntry\n"));
	NTSTATUS status = STATUS_SUCCESS;

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = CleanDispatchRoutine;

	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = WriteDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = QueryInfoDispatchRoutine;

	status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	

	KdPrint(("Leave DriverEntry\n"));

	return status;

}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Enter CreateDevice\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING ustrDevName;
	RtlInitUnicodeString(&ustrDevName, devName);

	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&ustrDevName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&pDevObj);
	if (!NT_SUCCESS(status))
	{
		/*KdPrint(("CreateDevice 失败 code: %d\n",status));*/
		if (status == STATUS_INSUFFICIENT_RESOURCES)
			KdPrint(("CreateDevice 失败 code: STATUS_INSUFFICIENT_RESOURCES\n"));
		else if(status == STATUS_OBJECT_NAME_COLLISION)
			KdPrint(("CreateDevice 失败 code: STATUS_OBJECT_NAME_COLLISION\n"));


		return status;
	}
	pDevObj->Flags |= DO_BUFFERED_IO;

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	
	pDevExt->pDeviceObj = pDevObj;

	UNICODE_STRING ustrSymLinkName;
	RtlInitUnicodeString(&ustrSymLinkName, symLinkName);
	

	status = IoCreateSymbolicLink(&ustrSymLinkName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("创建符号链接失败\n"));
		IoDeleteSymbolicLink(&ustrSymLinkName);
		status = IoCreateSymbolicLink(&ustrSymLinkName, &ustrDevName);
		if (!NT_SUCCESS(status))
		{
			IoDeleteDevice(pDevObj);

			return status;
		}
	}
	pDevExt->uMaxLength = 1024;
	pDevExt->uLength = 0;
	pDevExt->pBuffer = (PCHAR)ExAllocatePool(PagedPool, 1024);
	RtlZeroMemory(pDevExt->pBuffer, 1024);
	pDevExt->usymLinName.Buffer = (PWCH)ExAllocatePool(PagedPool, ustrSymLinkName.MaximumLength);
	RtlCopyUnicodeString(&pDevExt->usymLinName, &ustrSymLinkName);
	KdPrint(("Leave CreateDevice\n"));
	return status;
}

VOID HelloDDKUnload(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Enter Unload\n"));
	PDEVICE_OBJECT pDevObj = pDriverObject->DeviceObject;
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	UNICODE_STRING ustrSymboLinkName;
	RtlInitUnicodeString(&ustrSymboLinkName, symLinkName);

	IoDeleteSymbolicLink(&ustrSymboLinkName);
	ExFreePool(pDevExt->usymLinName.Buffer);

	ExFreePool(pDevExt->pBuffer);

	IoDeleteDevice(pDevObj);

	KdPrint(("Leave Unload\n"));
}



NTSTATUS ReadDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{

	KdPrint(("Enter ReadDispatchRoutine\n"));
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG ulReadLength = stack->Parameters.Read.Length < pDevExt->uLength ? stack->Parameters.Read.Length : pDevExt->uLength;

	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, pDevExt->pBuffer, ulReadLength);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = ulReadLength;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave ReadDispatchRoutine\n"));
	return STATUS_SUCCESS;
}

NTSTATUS WriteDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter WriteDispatchRoutine"));
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	ULONG ulSpaceLength = pDevExt->uMaxLength - pDevExt->uLength;

	ULONG ulWrite = ulSpaceLength < stack->Parameters.Write.Length? ulSpaceLength: stack->Parameters.Write.Length;

	RtlCopyMemory(pDevExt->pBuffer + pDevExt->uLength, pIrp->AssociatedIrp.SystemBuffer, ulWrite);

	pDevExt->uLength += ulWrite;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = ulWrite;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave WriteDispatchRoutine"));

	return STATUS_SUCCESS;
}

NTSTATUS QueryInfoDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter QueryInfoDispatchRoutine\n"));

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	//获取IO堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	//得到文件信息
	FILE_INFORMATION_CLASS info = stack->Parameters.QueryFile.FileInformationClass;
	//判断是否是标准文件信息
	if (info == FileStandardInformation)
	{
		KdPrint(("FileStandardInformation\n"));
		PFILE_STANDARD_INFORMATION file_info = (PFILE_STANDARD_INFORMATION)pIrp->AssociatedIrp.SystemBuffer;
		file_info->EndOfFile = RtlConvertLongToLargeInteger(pDevExt->uLength);
		KdPrint(("--%d++++++++++++---", pDevExt->uLength));
	}
	KdPrint(("---------%d-----------\n", stack->Parameters.QueryFile.Length));
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = stack->Parameters.QueryFile.Length;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave QueryInfoDispatchRoutine\n"));

	return STATUS_SUCCESS;
}

NTSTATUS HelloDDKDispatchRoutine(PDEVICE_OBJECT pDevice_Object, PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave HelloDDKDispatchRoutine\n"));

	return STATUS_SUCCESS;
}

NTSTATUS CloseDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter CloseDispatchRoutine\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave CloseDispatchRoutine\n"));

	return STATUS_SUCCESS;
}

NTSTATUS CleanDispatchRoutine(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	KdPrint(("Enter CleanDispatchRoutine\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("Leave CleanDispatchRoutine\n"));

	return STATUS_SUCCESS;
}


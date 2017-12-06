#include "DriverB.h"

#define DeviceNameA	L"\\Device\\MyDDKDeviceA"
#define LinkNameA	L"\\??\\HelloDDKA"

#define DeviceNameB		L"\\Device\\MyDDKDeviceB"
#define LinkNameB		L"\\??\\HelloDDKB"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS ntStatus;
	KdPrint(("DriverB: Enter B Driver"));

	pDriverObject->DriverUnload = HelloDDKUnload;
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = HelloDDKDispatchRoutine;
	}
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKRead;

	UNICODE_STRING uDeviceNameA;
	RtlInitUnicodeString(&uDeviceNameA, DeviceNameA);

	PDEVICE_OBJECT DeviceObject = NULL;
	PFILE_OBJECT	FileObject = NULL;
	ntStatus = IoGetDeviceObjectPointer(&uDeviceNameA,
		FILE_ALL_ACCESS,
		&FileObject,
		&DeviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("DriverB:IoGetDeviceObjectPointer() 0x%x\n", ntStatus));
		return ntStatus;
	}

	//创建自己的驱动设备对象
	ntStatus = CreateDevice(pDriverObject);

	if (!NT_SUCCESS(ntStatus))
	{
		ObDereferenceObject(FileObject);
		KdPrint(("IoCreateDevice() 0x%x\n", ntStatus));
		return ntStatus;
	}

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;

	PDEVICE_OBJECT FilterDeviceObject = pdx->pDevice;

	PDEVICE_OBJECT TargetDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, DeviceObject);

	//将底层设备对象记录下来
	pdx->TargetDevice = TargetDevice;

	if (!TargetDevice)
	{
		ObDereferenceObject(FileObject);
		IoDeleteDevice(FilterDeviceObject);
		KdPrint(("IoAttachDeviceToDeviceStack() 0x%x\n", ntStatus));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	FilterDeviceObject->DeviceType = TargetDevice->DeviceType;
	FilterDeviceObject->Characteristics = TargetDevice->Characteristics;
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	FilterDeviceObject->Flags |= (TargetDevice->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO));

	ObDereferenceObject(FileObject);

	KdPrint(("DriverB: B attached A successfully!\n"));
	KdPrint(("DriverB: Leave B DriverEntry\n"));

	return ntStatus;
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS ntStatus;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING	devName;
	RtlInitUnicodeString(&devName, DeviceNameB);

	//创建设备
	ntStatus = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0, TRUE,
		&pDevObj);
	if (!NT_SUCCESS(ntStatus))
		return ntStatus;

	pDevObj->Flags |= DO_DIRECT_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	return STATUS_SUCCESS;
}

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("DriverB:Enter B DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;

	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;
		pNextObj = pNextObj->NextDevice;
		//从设备栈中弹出
		IoDetachDevice(pDevExt->TargetDevice);
		//删除该设备对象
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("DriverB:Enter B DriverUnload\n"));
}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp)
{
	KdPrint(("DriverB:Enter B HelloDDKDispatchRoutine\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverB:Leave B HelloDDKDispatchRoutine\n"));
	return ntStatus;
}

NTSTATUS HelloDDKCreate(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("DriverB:Enter B HelloDDKCreate\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//
	// 	// 完成IRP
	// 	pIrp->IoStatus.Status = ntStatus;
	// 	pIrp->IoStatus.Information = 0;	// bytes xfered
	// 	IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB:Leave B HelloDDKCreate\n"));

	return ntStatus;
}

const int MAX_PACKAGE_SIZE = 1024;

NTSTATUS
HelloDDKReadCompletion(PDEVICE_OBJECT DeviceObject, PIRP pIrp, PVOID Context)
{
	KdPrint(("DriverB: Enter B HelloDDKReadCompletion\n"));

	PMYDRIVER_RW_CONTEXT rwContext = (PMYDRIVER_RW_CONTEXT)Context;
	NTSTATUS ntStatus = pIrp->IoStatus.Status;

	ULONG stageLength;
	if (rwContext && NT_SUCCESS(ntStatus))
	{
		rwContext->Numxfer += pIrp->IoStatus.Information;

		if (rwContext->Length)
		{
			if (rwContext->Length > MAX_PACKAGE_SIZE)
			{
				stageLength = MAX_PACKAGE_SIZE;
			}
			else
			{
				stageLength = rwContext->Length;
			}

			//重新利用MDL
			MmPrepareMdlForReuse(rwContext->NewMdl);

			IoBuildPartialMdl(pIrp->MdlAddress,
				rwContext->NewMdl,
				(PVOID)rwContext->VirtualAddress,
				stageLength);

			rwContext->VirtualAddress += stageLength;
			rwContext->Length -= stageLength;

			IoCopyCurrentIrpStackLocationToNext(pIrp);
			PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(pIrp);

			nextStack->Parameters.Read.Length = stageLength;

			IoSetCompletionRoutine(pIrp, HelloDDKReadCompletion,
				rwContext,
				TRUE,
				TRUE,
				TRUE);

			IoCallDriver(rwContext->DeviceExtension->TargetDevice, pIrp);

			return STATUS_MORE_PROCESSING_REQUIRED;
		}
		else
		{
			pIrp->IoStatus.Information = rwContext->Numxfer;
		}
	}
	KdPrint(("DriverB:Leave B HelloDDKReadCompletion\n"));
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS HelloDDKRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	KdPrint(("DriverB: Enter B HelloDDKRead\n"));
	NTSTATUS status = STATUS_SUCCESS;

	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	ULONG totalLength;
	ULONG stageLength;
	PMDL mdl;
	PVOID virtualAddress;
	PMYDRIVER_RW_CONTEXT rwContext = NULL;
	PIO_STACK_LOCATION nextStack;

	if (!pIrp->MdlAddress)
	{
		status = STATUS_UNSUCCESSFUL;
		totalLength = 0;
		goto HelloDDKRead_EXIT;
	}

	virtualAddress = MmGetMdlVirtualAddress(pIrp->MdlAddress);
	totalLength = MmGetMdlByteCount(pIrp->MdlAddress);

	KdPrint(("DriverB:(pIrp->MdlAddress)MmGetVirtualAddress:%08X\n", MmGetMdlVirtualAddress(pIrp->MdlAddress)));
	KdPrint(("DriverB:(pIrp->MdlLeng)MmGetByteCount:%08X\n", MmGetMdlByteCount(pIrp->MdlAddress)));

	//将总的传输，分成几个阶段，这里设定每次阶段的长度
	if (totalLength > MAX_PACKAGE_SIZE)
	{
		stageLength = MAX_PACKAGE_SIZE;
	}
	else
	{
		stageLength = totalLength;
	}

	mdl = IoAllocateMdl((PVOID)virtualAddress,
		totalLength,
		FALSE,
		FALSE,
		NULL);

	KdPrint(("DriverB:(new mdl)MmGetMdlVirtualAddress:%08X\n", MmGetMdlVirtualAddress(mdl)));
	KdPrint(("DriverB:(new mdl)MmGetMdlByteCount:%d\n", MmGetMdlByteCount(mdl)));

	if (mdl == NULL)
	{
		KdPrint(("DriverB:Failed to alloc mem for mdl\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto HelloDDKRead_EXIT;
	}

	//将IRP的MDL做重新映射
	IoBuildPartialMdl(pIrp->MdlAddress,
		mdl,
		(PVOID)virtualAddress,
		stageLength);

	KdPrint(("DriverB:(new mdl)MmGetMdlVirtualAddress:%08X\n", MmGetMdlVirtualAddress(mdl)));
	KdPrint(("DriverB:(new mdl)MmGetMdlByteCount:%d\n",MmGetMdlByteCount(mdl)));

	rwContext = (PMYDRIVER_RW_CONTEXT)ExAllocatePool(NonPagedPool, sizeof(MYDRIVER_RW_CONTEXT));

	rwContext->NewMdl = mdl;
	rwContext->PreviousMdl = pIrp->MdlAddress;
	rwContext->Length = totalLength - stageLength;
	rwContext->Numxfer = 0;
	rwContext->VirtualAddress = ((ULONG_PTR)virtualAddress + stageLength);
	rwContext->DeviceExtension = pDevExt;

	IoCopyCurrentIrpStackLocationToNext(pIrp);
	nextStack = IoGetNextIrpStackLocation(pIrp);

	nextStack->Parameters.Read.Length = stageLength;

	pIrp->MdlAddress = mdl;

	//设定完成例程
	IoSetCompletionRoutine(pIrp,
		(PIO_COMPLETION_ROUTINE)HelloDDKReadCompletion,
		rwContext,
		TRUE,
		TRUE,
		TRUE);

	IoCallDriver(pDevExt->TargetDevice, pIrp);

	pIrp->MdlAddress = rwContext->PreviousMdl;
	IoFreeMdl(rwContext->NewMdl);

HelloDDKRead_EXIT:
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = totalLength;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("DriverB:Leave B HelloDDKRead\n"));

	return status;
}

NTSTATUS HelloDDKClose(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("DriverB:Enter B HelloDDKClose\n"));
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	IoSkipCurrentIrpStackLocation(pIrp);

	ntStatus = IoCallDriver(pdx->TargetDevice, pIrp);

	KdPrint(("DriverB:Leave B HelloDDKClose\n"));

	return ntStatus;
}
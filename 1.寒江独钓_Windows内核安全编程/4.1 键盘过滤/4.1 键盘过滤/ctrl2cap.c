#include <wdm.h>
#include <ntddkbd.h>

#define KBD_DRIVER_NAME		L"\\Driver\\kbdclass"

typedef struct _C2P_DEV_EXT
{
	//这个结构的大小
	ULONG NodeSize;
	//过滤设备对象
	PDEVICE_OBJECT pFilterDeviceObject;
	//同时调用时的保护锁
	KSPIN_LOCK IoRequestsSpinLock;
	//进程间同步处理
	KEVENT IoInProgressEvent;
	//绑定的设备对象
	PDEVICE_OBJECT TargetDeviceObject;
	//绑定前底层设备对象
	PDEVICE_OBJECT LowerDeviceObject;
}C2P_DEV_EXT, *PC2P_DEV_EXT;

NTSTATUS
c2pDevExtInit(
	PC2P_DEV_EXT devExt,
	PDEVICE_OBJECT pFilterDeviceObject,
	PDEVICE_OBJECT pTargetDeviceObject,
	PDEVICE_OBJECT pLowerDeviceObject)
{
	memset(devExt, 0, sizeof(C2P_DEV_EXT));
	devExt->NodeSize = sizeof(C2P_DEV_EXT);
	devExt->pFilterDeviceObject = pFilterDeviceObject;
	KeInitializeSpinLock(&(devExt->IoRequestsSpinLock));
	KeInitializeEvent(&(devExt->IoInProgressEvent), NotificationEvent, FALSE);
	devExt->TargetDeviceObject = pTargetDeviceObject;
	devExt->LowerDeviceObject = pLowerDeviceObject;
	return (STATUS_SUCCESS);
}

//这个函数未在文档中公开,声明一下就可以使用
NTSTATUS ObReferenceObjectByName(
	PUNICODE_STRING ObjectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContext,
	PVOID *Object);

extern POBJECT_TYPE IoDriverObjectType;
ULONG gC2pKeyCount = 0;
PDRIVER_OBJECT gDriverObject = NULL;

NTSTATUS
c2pAttachDevices(PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = 0;
	UNICODE_STRING uniNtNameString;
	PC2P_DEV_EXT devExt;
	PDEVICE_OBJECT pFilterDeviceObject = NULL;
	PDEVICE_OBJECT pTargetDeviceObject = NULL;
	PDEVICE_OBJECT pLowerDeviceObject = NULL;

	PDRIVER_OBJECT KbdDriverObject = NULL;

	KdPrint(("MyAttach\n"));

	//
	RtlInitUnicodeString(&uniNtNameString, KBD_DRIVER_NAME);
	status = ObReferenceObjectByName(
		&uniNtNameString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		FILE_ALL_ACCESS,
		IoDriverObjectType,
		KernelMode,
		NULL,
		&KbdDriverObject
	);

	//如果失败就直接返回
	if (!NT_SUCCESS(status))
	{
		KdPrint(("MyAttach: Couldn't get the MyTest Device Object: 0X%X\n", status));
		return status;
	}
	else
	{
		ObDereferenceObject(DriverObject);
	}

	pTargetDeviceObject = KbdDriverObject->DeviceObject;

	while (pTargetDeviceObject)
	{
		status = IoCreateDevice(
			DriverObject,
			sizeof(C2P_DEV_EXT),
			NULL,
			pTargetDeviceObject->DeviceType,
			pTargetDeviceObject->Characteristics,
			FALSE,
			&pFilterDeviceObject
		);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("MyAttach: Couldn't create the MyFilter Filter Device Object\n"));
			return (status);
		}

		pLowerDeviceObject = IoAttachDeviceToDeviceStack(pFilterDeviceObject, pTargetDeviceObject);
		//如果绑定失败了，放弃之前的操作
		if (!pLowerDeviceObject)
		{
			KdPrint(("MyAttach: Couldn't attach to MyTest Device Object\n"));
			IoDeleteDevice(pFilterDeviceObject);
			pFilterDeviceObject = NULL;
			return status;
		}

		devExt = (PC2P_DEV_EXT)(pFilterDeviceObject->DeviceExtension);
		c2pDevExtInit(devExt,
			pFilterDeviceObject,
			pTargetDeviceObject,
			pLowerDeviceObject);

		pFilterDeviceObject->DeviceType = pLowerDeviceObject->DeviceType;
		pFilterDeviceObject->Characteristics = pLowerDeviceObject->Characteristics;
		pFilterDeviceObject->StackSize = pLowerDeviceObject->StackSize + 1;
		pFilterDeviceObject->Flags |= pLowerDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);

		pTargetDeviceObject = pTargetDeviceObject->NextDevice;
	}
	return status;
}

VOID c2pDetach(PDEVICE_OBJECT pDeviceObject)
{
	PC2P_DEV_EXT devExt;
	BOOLEAN NoRequestsOutstanding = FALSE;
	devExt = (PC2P_DEV_EXT)pDeviceObject->DeviceExtension;
	__try
	{
		__try
		{
			IoDetachDevice(devExt->TargetDeviceObject);
			devExt->TargetDeviceObject = NULL;
			IoDeleteDevice(pDeviceObject);
			devExt->pFilterDeviceObject = NULL;
			KdPrint(("Detach Finished\n"));
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}
	__finally {}
	return;
}

#define DELAY_ONE_MICROSECOND	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)
#define DELAY_ONE_SECOND		(DELAY_ONE_MILLISECOND*1000)

VOID
c2pUnload(PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT OldDeviceObject;
	PC2P_DEV_EXT devExt;

	LARGE_INTEGER	lDelay;
	PRKTHREAD CurrentThread;

	lDelay = RtlConvertLongToLargeInteger(100 * DELAY_ONE_MILLISECOND);
	CurrentThread = KeGetCurrentThread();
	//把当前线程设置为低实时模式，以便让它运行尽量少影响其他程序
	KeSetPriorityThread(CurrentThread, LOW_REALTIME_PRIORITY);

	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("DriverEntry unLoading...\n"));

	//遍历所有设备并一律解除绑定
	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject)
	{
		c2pDetach(DeviceObject);
		DeviceObject = DeviceObject->NextDevice;
	}
	ASSERT(NULL == DriverObject->DeviceObject);

	while (gC2pKeyCount)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &lDelay);
	}
	KdPrint(("DriverEntry unLoad OK!\n"));
	return;
}

NTSTATUS c2pDispatchGeneral(PDEVICE_OBJECT DeviceObject, PIRP pIrp)
{
	KdPrint(("Other Dispatch!\n"));
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(((PC2P_DEV_EXT)DeviceObject->DeviceExtension)->LowerDeviceObject, pIrp);
}

NTSTATUS c2pPower(PDEVICE_OBJECT DeviceObject, PIRP pIrp)
{
	PC2P_DEV_EXT devExt;
	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;

	PoStartNextPowerIrp(pIrp);
	IoSkipCurrentIrpStackLocation(pIrp);
	return PoCallDriver(devExt->LowerDeviceObject, pIrp);
}

NTSTATUS c2pPnp(PDEVICE_OBJECT DeviceObject, PIRP pIrp)
{
	PC2P_DEV_EXT devExt;
	PIO_STACK_LOCATION irpStack;
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL oldIrql;
	KEVENT event;

	//获得真实设备
	devExt = (PC2P_DEV_EXT)(DeviceObject->DeviceExtension);
	irpStack = IoGetCurrentIrpStackLocation(pIrp);

	switch (irpStack->MinorFunction)
	{
	case IRP_MN_REMOVE_DEVICE:
	{
		KdPrint(("IRP_MN_REMOVE_DEVICE\n"));

		//先把请求发下去
		IoSkipCurrentIrpStackLocation(pIrp);
		IoCallDriver(devExt->LowerDeviceObject, pIrp);
		//然后解除绑定
		IoDetachDevice(devExt->LowerDeviceObject);
		//删除我们自己生成的虚拟设备
		IoDeleteDevice(DeviceObject);
		status = STATUS_SUCCESS;
		break;
	}
	default:
	{
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(devExt->LowerDeviceObject, pIrp);
	}
	}
	return status;
}

#define S_SHIFT		1
#define S_CAPS		2
#define	S_NUM		4

static int kb_status = S_NUM;
void __stdcall print_keystroke(UCHAR sch)
{
	UCHAR	ch = 0;
	int		off = 0;

	if ((sch & 0x80) == 0)  //如果是按下
	{
		//如果按下了字母或者数字等可见字符
		/*if ((sch < 0x47) || ((sch >= 0x47 && sch < 0x54) && (kb_status & S_NUM)))
		{
			ch = 
		}*/
	}
}




NTSTATUS c2pReadComplete(PDEVICE_OBJECT DeviceObject,
	PIRP pIrp,
	PVOID Context)
{
	PIO_STACK_LOCATION IrpSp;
	ULONG buf_len = 0;
	PUCHAR buf = NULL;
	size_t i;

	IrpSp = IoGetCurrentIrpStackLocation(pIrp);

	if (NT_SUCCESS(pIrp->IoStatus.Status))
	{
		PKEYBOARD_INPUT_DATA KeyData;

		KeyData = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;

		buf_len = pIrp->IoStatus.Information/sizeof(KEYBOARD_INPUT_DATA);

		//打印所有的扫描码
		for (i = 0; i < buf_len; ++i)
		{
			KdPrint(("numKeys: %d\n", buf_len));
			KdPrint(("ScanCode: %x ", KeyData->MakeCode));
			KdPrint(("%s\n", KeyData->Flags ? "Up" : "Down"));

			//KdPrint(("ctrl2cap: %2x\n", buf[i]));
		}
	}
	gC2pKeyCount--;

	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}
	return pIrp->IoStatus.Status;
}

NTSTATUS c2pDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PC2P_DEV_EXT devExt;
	PIO_STACK_LOCATION currentIrpStack;
	KEVENT waitEvent;
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);

	if (pIrp->CurrentLocation == 1)
	{
		ULONG ReturnedInformation = 0;
		KdPrint(("Dispatch encountere bogus current location\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		pIrp->IoStatus.Status = status;
		pIrp->IoStatus.Information = ReturnedInformation;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return (status);
	}

	gC2pKeyCount++;

	devExt = (PC2P_DEV_EXT)DeviceObject->DeviceExtension;

	//设置回调函数并把IRP传递下去。之后读的处理也就结束了。
	//剩下的任务是要等待请求完成
	currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	IoSetCompletionRoutine(pIrp, c2pReadComplete, DeviceObject, TRUE, TRUE, TRUE);

	return IoCallDriver(devExt->LowerDeviceObject, pIrp);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	ULONG i;
	NTSTATUS status;
	KdPrint(("c2p.Sys: entering DriverEntry\n"));

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		DriverObject->MajorFunction[i] = c2pDispatchGeneral;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = c2pDispatchRead;

	DriverObject->MajorFunction[IRP_MJ_POWER] = c2pPower;

	//IRP_MJ_PNP 即插即用分发函数
	DriverObject->MajorFunction[IRP_MJ_PNP] = c2pPnp;

	//卸载函数
	DriverObject->DriverUnload = c2pUnload;
	gDriverObject = DriverObject;

	status = c2pAttachDevices(DriverObject, RegistryPath);
	return status;
}

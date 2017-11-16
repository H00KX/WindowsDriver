#include "Driver.h"

VOID StringInitTest()
{
	//1. 用RtlInitAnsiString初始化字符串
	ANSI_STRING AnsiString1;
	CHAR string1[] = "hello";
	//初始化ANSI_STRING
	RtlInitAnsiString(&AnsiString1, string1);
	KdPrint(("InitTest AnsiString1:%Z\n", &AnsiString1));

	string1[0] = 'H';
	string1[1] = 'E';
	string1[2] = 'L';
	string1[3] = 'L';
	string1[4] = 'O';

	KdPrint(("InitTest AnsiString1:%Z\n", &AnsiString1));


	//2. 程序员自己初始化字符串
#define BUFFER_SIZE		1024
	UNICODE_STRING UnicodeString1 = { 0 };
	//设置缓冲区大小
	UnicodeString1.MaximumLength = BUFFER_SIZE;
	//分配内存
	UnicodeString1.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	WCHAR* wideString = L"hello";

	//设置字符长度，宽字符是字符长度的2倍
	UnicodeString1.Length = 2 * wcslen(wideString);

	//保证缓冲区足够大，否则程序终止
	ASSERT(UnicodeString1.MaximumLength >= UnicodeString1.Length);
	//内存拷贝
	RtlCopyMemory(UnicodeString1.Buffer, wideString, UnicodeString1.Length);

	KdPrint(("程序员自己初始化字符串: %wZ\n", &UnicodeString1));

	//清理内存
	ExFreePool(UnicodeString1.Buffer);
	UnicodeString1.Buffer = NULL;
	UnicodeString1.Length = UnicodeString1.MaximumLength = 0;
}

VOID StringCopyTest()
{
	//初始化UnicodeString1
	UNICODE_STRING UnicodeString1;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");

	//初始化UnicodeString2
	UNICODE_STRING UnicodeString2 = { 0 };
	UnicodeString2.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	UnicodeString2.MaximumLength = BUFFER_SIZE;

	//通过拷贝UnicodeString1初始化UnicodeString2
	RtlCopyUnicodeString(&UnicodeString2, &UnicodeString1);

	//分别显示UnicodeString1和UnicodeString2
	KdPrint(("StringCopyTest UnicodeString1:%wZ\n", &UnicodeString1));
	KdPrint(("StringCopyTest UnicodeString2:%wZ\n", &UnicodeString2));

	//销毁UnicodeString2
	//UnicodeString1不需要销毁
	RtlFreeUnicodeString(&UnicodeString2);
}

VOID StringCompareTest()
{
	//初始化UnicodeString1
	UNICODE_STRING UnicodeString1;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");

	//初始化UnicodeString2
	UNICODE_STRING UnicodeString2;
	RtlInitUnicodeString(&UnicodeString2, L"Hello");

	if (RtlEqualUnicodeString(&UnicodeString1, &UnicodeString2, TRUE))
	{
		KdPrint(("UnicodeString1 and UnicodeString2 are equal\n"));
	}
	else
	{
		KdPrint(("UnicodeString1 and UnicodeString2 are Not equal\n"));
	}
}

VOID StringToUpperTest()
{
	//初始化UnicodeString1
	UNICODE_STRING UnicodeString1;
	WCHAR ustr1[] = L"Hello World";
	RtlInitUnicodeString(&UnicodeString1, ustr1);

	//变化前
	KdPrint(("StringToUpperTest UnicodeString1: %wZ\n", &UnicodeString1));

	//变大写
	RtlUpcaseUnicodeString(&UnicodeString1, &UnicodeString1, FALSE);

	//变化后
	KdPrint(("StringToUpperTest UnicodeString1:%wZ\n", &UnicodeString1));
}

VOID StringToIntegerTest()
{
	//(1) 字符串转换成数字
	//初始化UnicodeString1
	UNICODE_STRING UnicodeString1;
	RtlInitUnicodeString(&UnicodeString1, L"-100");

	ULONG lNumber;
	NTSTATUS nStatus = RtlUnicodeStringToInteger(&UnicodeString1, 10, &lNumber);
	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("Conver to integer successfully!\n"));
		KdPrint(("Result:%d\n", lNumber));
	}
	else
	{
		KdPrint(("Conver to integer unsuccessfully!\n"));
	}

	//(2) 数字转换成字符串
	//初始化UnicodeString2
	UNICODE_STRING UnicodeString2 = { 0 };
	UnicodeString2.Buffer = (PWSTR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	UnicodeString2.MaximumLength = BUFFER_SIZE;
	nStatus = RtlIntegerToUnicodeString(200, 10, &UnicodeString2);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("Conver to string successfully\n"));
		KdPrint(("Result:%wZ\n", &UnicodeString2));
	}
	else
	{
		KdPrint(("Conver to string unsuccessfully!\n"));
	}
	//销毁UnicodeString2
	//注意!UnicodeString1不用销毁
	RtlFreeUnicodeString(&UnicodeString2);
}

VOID StringConverTest()
{
	//1. 将UNICODE_STRING 字符串转换成 ANSI_STRING
	//初始化UnicodeString1
	UNICODE_STRING UnicodeString1;
	RtlInitUnicodeString(&UnicodeString1, L"Hello World");

	ANSI_STRING AnsiString1;
	NTSTATUS nStatus = RtlUnicodeStringToAnsiString(&AnsiString1, &UnicodeString1, TRUE);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("Conver UnicodeToAnsi successfully!\n"));
		KdPrint(("Result:%Z\n", &AnsiString1));
	}
	else
	{
		KdPrint(("Conver UnicodeToAnsi unsuccessfully!\n"));
	}

	//销毁AnsiString1
	RtlFreeAnsiString(&AnsiString1);

	//2. 将ANSI_STRING字符串转换成UNICODE_STRING字符串
	ANSI_STRING AnsiString2;
	RtlInitString(&AnsiString2, "Hello World");

	UNICODE_STRING UnicodeString2;
	nStatus = RtlAnsiStringToUnicodeString(&UnicodeString2, &AnsiString2, TRUE);
	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("AnsiToUnicodeString Conver successfully!\n"));
		KdPrint(("Result:%wZ\n", &UnicodeString2));
	}
	else
	{
		KdPrint(("AnsiToUnicodeString unsuccessfully!\n"));
	}

	//销毁UnicodeString2
	RtlFreeUnicodeString(&UnicodeString2);
}

VOID StringTest()
{
	StringInitTest();

	StringCopyTest();

	StringCompareTest();

	StringToUpperTest();

	StringToIntegerTest();

	StringConverTest();
}

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath)
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

	StringTest();

	KdPrint(("DriverEntry end\n"));
	return status;
}

/************************************************************************
* 函数名称:CreateDevice
* 功能描述:初始化设备对象
* 参数列表:
pDriverObject:从I/O管理器中传进来的驱动对象
* 返回 值:返回初始化状态
*************************************************************************/

NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT	pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

	//创建设备
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&(UNICODE_STRING)devName,
		FILE_DEVICE_UNKNOWN,
		0, TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;
	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK");
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}

/************************************************************************
* 函数名称:HelloDDKUnload
* 功能描述:负责驱动程序的卸载操作
* 参数列表:
pDriverObject:驱动对象
* 返回 值:返回状态
*************************************************************************/

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;

		//删除符号链接
		UNICODE_STRING pLinkName;
		UNICODE_STRING symLinkName;
		RtlInitUnicodeString(&symLinkName, L"\\??\\HelloDDK");

		IoDeleteSymbolicLink(&symLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
}

/************************************************************************
* 函数名称:HelloDDKDispatchRoutine
* 功能描述:对读IRP进行处理
* 参数列表:
pDevObj:功能设备对象
pIrp:从IO请求包
* 返回 值:返回状态
*************************************************************************/

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

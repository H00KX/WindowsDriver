#pragma once
#include "Driver.h"

VOID CreateFileTest()
{
	OBJECT_ATTRIBUTES	objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	RtlInitUnicodeString(&logFileUnicodeString, L"\\??\\C:\\1.log");
	//或者写成 "\\Device\\HarddiskVolume1\\1.LOG"

	//初始化objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//创建文件
	NTSTATUS ntStatus = ZwCreateFile(&hfile,
		GENERIC_WRITE,
		&objectAttributes,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("Create file successfully!\n"));
	}
	else
	{
		KdPrint(("Create file unsuccessfully!\n"));
	}

	//文件操作
	//.......

	//关闭文件句柄
	ZwClose(hfile);
}

//获取进程名
VOID DispatchProcess()
{
	//得到当前进程
	PEPROCESS pEProcess = PsGetCurrentProcess();
	//得到当前进程名称
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);
	KdPrint(("%s\n", ProcessName));
}

VOID OpenFileTest1()
{
	OBJECT_ATTRIBUTES	objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	RtlInitUnicodeString(&logFileUnicodeString, L"\\??\\C:\\1.log");
	//或者写成 "\\Device\\HarddiskVolume1\\1.LOG"

	//初始化 objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	NTSTATUS ntStatus = ZwCreateFile(&hfile,
		GENERIC_READ,
		&objectAttributes,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("Open file successfully!\n"));
	}
	else
	{
		KdPrint(("Open file unsuccessfully!\n"));
	}

	//文件操作
	//.................

	//关闭文件句柄

	ZwClose(hfile);
}

VOID FileAttributeTest()
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	//
	RtlInitUnicodeString(&logFileUnicodeString,
		L"\\??\\C:\\1.log");

	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	//创建文件
	NTSTATUS ntStatus = ZwCreateFile(&hfile,
		GENERIC_READ,
		&objectAttributes,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("open file successfully.\n"));
	}
	FILE_STANDARD_INFORMATION fsi;
	ntStatus = ZwQueryInformationFile(hfile,
		&iostatus,
		&fsi,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("file length: %u\n", fsi.EndOfFile.QuadPart));
	}

	//修改当前文件指针
	FILE_POSITION_INFORMATION fpi;
	fpi.CurrentByteOffset.QuadPart = 100i64;
	ntStatus = ZwSetInformationFile(hfile,
		&iostatus,
		&fpi,
		sizeof(FILE_POSITION_INFORMATION),
		FilePositionInformation);
	if (NT_SUCCESS(ntStatus))
	{
		KdPrint(("update the file pointer successfully.\n"));
	}

	//关闭文件句柄
	ZwClose(hfile);
}

VOID WriteFileTest()
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	//初始化UNICODE_STRING字符串
	RtlInitUnicodeString(&logFileUnicodeString,
		L"\\??\\C:\\1.log");
	//初始化
	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,    //对大小写敏感
		NULL,
		NULL);
	
	//创建文件
	NTSTATUS ntStatus = ZwCreateFile(
		&hfile,
		GENERIC_WRITE,
		&objectAttributes,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
#define	BUFFER_SIZE		1024
	PUCHAR pBuffer = (PUCHAR)ExAllocatePool(PagedPool, BUFFER_SIZE);
	//构造要填充的数据
	RtlFillMemory(pBuffer, BUFFER_SIZE, 0xAA);

	KdPrint(("The program will write %d bytes\n", BUFFER_SIZE));
	//写文件
	ZwWriteFile(hfile, NULL, NULL, NULL, &iostatus, pBuffer, BUFFER_SIZE, NULL, NULL);
	KdPrint(("The program really wrote %d bytes\n", iostatus.Information));

	//构造要填充的数据
	RtlFillMemory(pBuffer, BUFFER_SIZE, 0xBB);
	
	KdPrint(("The program will append %d bytes\n", BUFFER_SIZE));
	//追加数据
	LARGE_INTEGER number;
	number.QuadPart = 1024i64;  //设置文件指针
	//对文件进行附加写
	ZwWriteFile(hfile, NULL, NULL, NULL, &iostatus, pBuffer, BUFFER_SIZE, &number, NULL);
	KdPrint(("The program really appended % bytes\n", iostatus.Information));

	//关闭文件句柄
	ZwClose(hfile);

	ExFreePool(pBuffer);
}

VOID ReadFileTest()
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	RtlInitUnicodeString(&logFileUnicodeString, L"\\??\\C:\\1.log");

	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//创建文件
	NTSTATUS ntStatus = ZwCreateFile(&hfile,
		GENERIC_READ,
		&objectAttributes,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("The file is not exist!\n"));
		return;
	}

	FILE_STANDARD_INFORMATION fsi;
	//读取文件长度
	ntStatus = ZwQueryInformationFile(
		hfile,
		&iostatus,
		&fsi,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation);

	KdPrint(("The program want to read %d bytes\n", fsi.EndOfFile.QuadPart));

	//为读取的文件分配缓冲区
	PUCHAR pBuffer = (PUCHAR)ExAllocatePool(PagedPool,
		(LONG)fsi.EndOfFile.QuadPart);

	//读取文件
	ZwReadFile(hfile, NULL,
		NULL, NULL, &iostatus,
		pBuffer,
		(LONG)fsi.EndOfFile.QuadPart,
		NULL,
		NULL);
	KdPrint(("The program really read %d bytes\n", iostatus.Information));
	//关闭文件句柄
	ZwClose(hfile);

	//释放缓冲区
	ExFreePool(pBuffer);
}

VOID FileTest()
{
	//创建文件实验
	CreateFileTest();

	//打开文件实验
	OpenFileTest1();
	
	FileAttributeTest();

	WriteFileTest();

	ReadFileTest();

	DispatchProcess();
}


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	FileTest();

	KdPrint(("DriverEntry end\n"));

	return STATUS_SUCCESS;
}


NTSTATUS	CreateDevice(IN PDRIVER_OBJECT pDriverObject)
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

VOID	HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//删除符号链接
		UNICODE_STRING pLinkName;
		RtlInitUnicodeString(&pLinkName, L"\\??\\HelloDDK");
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pDevExt->pDevice->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);

		
	}
}

NTSTATUS	HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0; //bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}


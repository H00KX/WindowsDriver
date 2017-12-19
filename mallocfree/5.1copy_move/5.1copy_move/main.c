#include <ntddk.h>

#define DeviceName L"\\Device\\HelloDDK"
#define LinkName	L"\\??\\HelloDDK"

NTSTATUS ntCopyFile(PWCHAR src, PWCHAR dst)
{
	UNICODE_STRING uSrcFile;
	UNICODE_STRING uDstFile;
	IO_STATUS_BLOCK iostatus;
	HANDLE hSrcFile;
	HANDLE hDstFile;
	NTSTATUS status;
	ULONG length = 0;

	OBJECT_ATTRIBUTES objSrcAttr;
	OBJECT_ATTRIBUTES objDstAttr;

	//PVOID buff = NULL;
	UCHAR buff[PAGE_SIZE] = { 0 };
	LARGE_INTEGER filepos = { 0 };

	RtlInitUnicodeString(&uSrcFile, src);
	RtlInitUnicodeString(&uDstFile, dst);

	InitializeObjectAttributes(&objSrcAttr,
		&uSrcFile,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	InitializeObjectAttributes(&objDstAttr,
		&uDstFile,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = ZwCreateFile(&hSrcFile,
		GENERIC_READ,
		&objSrcAttr,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwCreateFile failure %wZ, Code: %X\n", &uSrcFile, status));
		return status;
	}
	status = ZwCreateFile(&hDstFile,
		GENERIC_WRITE,
		&objDstAttr,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (!NT_SUCCESS(status))
	{
		ZwClose(hSrcFile);
		KdPrint(("ZwCreateFile failure %wZ, Code: %X\n", &uDstFile, status));
		return status;
	}
	
	/*buff = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'ELIF');
	if (buff == NULL)
	{
		ZwClose(hSrcFile);
		ZwClose(hDstFile);
		return STATUS_INSUFFICIENT_RESOURCES;
	}*/

	while (1)
	{
		status = ZwReadFile(hSrcFile,
			NULL,
			NULL,
			NULL,
			&iostatus,
			buff,
			PAGE_SIZE,
			&filepos,
			0);

		if (status == STATUS_END_OF_FILE)
		{
			
			KdPrint(("Src: %wZ Dst:%wZ 复制成功\n",&uSrcFile, &uDstFile));
			status = STATUS_SUCCESS;
			break;
		}
		length = (ULONG)iostatus.Information;

		status = ZwWriteFile(
			hDstFile,
			NULL,
			NULL,
			NULL,
			&iostatus,
			buff,
			length,
			&filepos,
			NULL
		);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("复制文件时失败:%x\n", status));
			break;
		}

		filepos.QuadPart += length;
	}

	//ExFreePool(buff);

	ZwClose(hSrcFile);
	ZwClose(hDstFile);

	return status;
}

NTSTATUS ntDeleteFile(PWCHAR src)
{
	UNICODE_STRING uFile;
	IO_STATUS_BLOCK iostatus;
	OBJECT_ATTRIBUTES objAttr;
	HANDLE hFile = NULL;
	FILE_DISPOSITION_INFORMATION disInfo;
	NTSTATUS status;
	FILE_NETWORK_OPEN_INFORMATION netInfo;
	FILE_BASIC_INFORMATION baseInfo;

	RtlInitUnicodeString(&uFile, src);
	InitializeObjectAttributes(&objAttr,
		&uFile,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = ZwCreateFile(&hFile,
		GENERIC_WRITE | DELETE,
		&objAttr,
		&iostatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);

	if (!NT_SUCCESS(status))
	{
		if (status == STATUS_ACCESS_DENIED)
		{
			status = ZwCreateFile(&hFile,
				GENERIC_READ | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES,
				&objAttr,
				&iostatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0);
			if (!NT_SUCCESS(status))
			{
				KdPrint(("设置属性，打开文件:%wZ 失败, status: %x\n", &uFile, status));
				return status;
			}
			ZwQueryInformationFile(
				hFile,
				&iostatus,
				&baseInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation
			);

			baseInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;

			status = ZwSetInformationFile(
				hFile,
				&iostatus,
				&baseInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation
			);

			if (!NT_SUCCESS(status))
			{
				KdPrint(("文件:%wZ 去除只读属性失败, status: %x\n", &uFile, status));
				return status;
			}

			ZwClose(hFile);

			status = ZwCreateFile(&hFile,
				GENERIC_WRITE | DELETE,
				&objAttr,
				&iostatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0);
			if (!NT_SUCCESS(status))
			{
				KdPrint(("以删除权限打开文件:%wZ 失败，code:%x\n", &uFile, status));
				return status;
			}

		}
	}

	disInfo.DeleteFile = TRUE;

	status = ZwSetInformationFile(hFile,
		&iostatus,
		&disInfo,
		sizeof(FILE_DISPOSITION_INFORMATION),
		FileDispositionInformation);

	if (!NT_SUCCESS(status))
	{
		ZwClose(hFile);
		KdPrint(("删除文件:%wZ 失败, status: %x\n", &uFile, status));
		return status;
	}
	ZwClose(hFile);
	KdPrint(("删除文件:%wZ 成功\n", &uFile));
	return status;
}

NTSTATUS ntMoveFile(PWCHAR src, PWCHAR dst)
{
	NTSTATUS status;
	status = ntCopyFile(src, dst);
	/*if (!NT_SUCCESS(status))
	{
		KdPrint(("移动文件不成功: %X\n", status));
		return status;
	}*/

	status = ntDeleteFile(src);
	return status;
}




VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING uSymbolic;
	RtlInitUnicodeString(&uSymbolic, LinkName);
	IoDeleteSymbolicLink(&uSymbolic);

	IoDeleteDevice(pDriverObject->DeviceObject);
}

VOID FileOper(VOID)
{
	PWCHAR src1 = L"\\??\\C:\\1.txt";
	PWCHAR src2 = L"\\??\\C:\\2.txt";
	PWCHAR src3 = L"\\??\\C:\\3.txt";

	ntCopyFile(src1, src2);
	ntMoveFile(src2, src3);
}

NTSTATUS	CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;

	//创建设备名称
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DeviceName);

	//创建设备
	status = IoCreateDevice(pDriverObject,
		0,
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	

	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, LinkName);
	
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	pDriverObject->DriverUnload = DriverUnload;
	CreateDevice(pDriverObject);
	FileOper();

	return STATUS_SUCCESS;
}


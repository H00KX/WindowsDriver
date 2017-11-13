#pragma once
#include "Driver.h"

VOID CreateFileTest()
{
	OBJECT_ATTRIBUTES	objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	RtlInitUnicodeString(&logFileUnicodeString, L"\\??\\C:\\1.log");
	//����д�� "\\Device\\HarddiskVolume1\\1.LOG"

	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//�����ļ�
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

	//�ļ�����
	//.......

	//�ر��ļ����
	ZwClose(hfile);
}

VOID OpenFileTest1()
{
	OBJECT_ATTRIBUTES	objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	RtlInitUnicodeString(&logFileUnicodeString, L"\\??\\C:\\1.log");
	//����д�� "\\Device\\HarddiskVolume1\\1.LOG"

	//��ʼ�� objectAttributes
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

	//�ļ�����
	//.................

	//�ر��ļ����

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

	//�����ļ�
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

	//�޸ĵ�ǰ�ļ�ָ��
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

	//�ر��ļ����
	ZwClose(hfile);
}

VOID WriteFileTest()
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK	iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&logFileUnicodeString,
		L"\\??\\C:\\1.log");
	//��ʼ��
	InitializeObjectAttributes(&objectAttributes,
		&logFileUnicodeString,
		OBJ_CASE_INSENSITIVE,    //�Դ�Сд����
		NULL,
		NULL);
	
	//�����ļ�
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
	//����Ҫ��������
	RtlFillMemory(pBuffer, BUFFER_SIZE, 0xAA);

	KdPrint(("The program will write %d bytes\n", BUFFER_SIZE));
	//д�ļ�
	ZwWriteFile(hfile, NULL, NULL, NULL, &iostatus, pBuffer, BUFFER_SIZE, NULL, NULL);
	KdPrint(("The program really wrote %d bytes\n", iostatus.Information));

	//����Ҫ��������
	RtlFillMemory(pBuffer, BUFFER_SIZE, 0xBB);
	
	KdPrint(("The program will append %d bytes\n", BUFFER_SIZE));
	//׷������
	LARGE_INTEGER number;
	number.QuadPart = 1024i64;  //�����ļ�ָ��
	//���ļ����и���д
	ZwWriteFile(hfile, NULL, NULL, NULL, &iostatus, pBuffer, BUFFER_SIZE, &number, NULL);
	KdPrint(("The program really appended % bytes\n", iostatus.Information));

	//�ر��ļ����
	ZwClose(hfile);

	ExFreePool(pBuffer);
}

VOID ReadFileTest()
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK iostatus;
	HANDLE hfile;
	UNICODE_STRING logFileUnicodeString;


}


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{


	return STATUS_SUCCESS;
}


NTSTATUS	CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{

	return STATUS_SUCCESS;
}

VOID	HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{

}

NTSTATUS	HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{


	return STATUS_SUCCESS;
}


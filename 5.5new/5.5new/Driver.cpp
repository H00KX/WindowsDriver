#pragma once
#include "Driver.h"

/***********ȫ��new������********************************************/
void * __cdecl operator new(size_t size, POOL_TYPE PoolType = PagedPool)
{
	KdPrint(("global operator new\n"));
	KdPrint(("Allocate size:%d\n", size));
	return ExAllocatePool(PoolType, size);
}
//ȫ��delete������
void __cdecl operator delete(void* pointer)
{
	KdPrint(("Global delete operator\n"));
	ExFreePool(pointer);
}

void __cdecl operator delete(void* pointer,unsigned int n)
{
	KdPrint(("Global delete operator%d\n",n));
	ExFreePool(pointer);
	
}

void __cdecl operator delete[](void* pointer)
{
	KdPrint(("Global delete operator----\n"));
	ExFreePool(pointer);
}

class TestClass
{
public:
	//���캯��
	TestClass()
	{
		KdPrint(("TestClass::TestClass()\n"));
	}
	~TestClass()
	{
		KdPrint(("TestClass::~TestClass()\n"));
	}

	//���е�new������
	void* operator new(size_t size, POOL_TYPE PoolType = PagedPool)
	{
		KdPrint(("TestClass::new\n"));
		KdPrint(("Allocate size :%d\n"));
		return ExAllocatePool(PoolType, size);
	}
	void operator delete(void* pointer)
	{
		KdPrint(("TestClass::delete\n"));
		ExFreePool(pointer);
	}
private:
	char buffer[1024];

};


void TestNewOperator()
{
	TestClass* pTestClass = new TestClass;
	delete pTestClass;

	pTestClass = new(NonPagedPool)TestClass;
	delete pTestClass;

	char *pBuffer = new(PagedPool) char[100];
	delete []pBuffer;

	pBuffer = new(NonPagedPool) char[100];
	delete []pBuffer;
}


extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT pDriverObject,
	IN PUNICODE_STRING pRegistryPath
)
{
	NTSTATUS status;
	KdPrint(("Enter DriverEntry\n"));

	//ע�������������ú������
	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;

	//���������豸����
	status = CreateDevice(pDriverObject);

	TestNewOperator();

	KdPrint(("DriverEntry end\n"));
	return status;
}

/****************************
�������ƣ�CreateDevice
������������ʼ���豸����
* �����б�
pDriverObject: ��I/O�������д���������������
* ����ֵ�����س�ʼ��״̬
*****************************/

NTSTATUS CreateDevice(
	IN PDRIVER_OBJECT pDriverObject
)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	//�����豸����
	UNICODE_STRING devName;

	RtlInitUnicodeString(&devName, L"\\Device\\MyDDKDevice");

	//�����豸
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
	//������������
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

/*********************
* �������ƣ�HelloDDKUnload
* �����������������������ж�ز���
* �����б�
*	pDriverObject: ��������
*
* ����ֵ������״̬
*****************************************************************/

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)                         //�����豸���󣬲�ɾ��
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//ɾ����������
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
}

/**************
* �������ƣ�HelloDDKDispatchRoutine
* �����������Զ�IRP���д���
* �����б�
*	pDevObj: �����豸����
*   pIrp����I/O�����
*����ֵ������״̬
**************************/

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

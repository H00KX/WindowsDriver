
#include <ntddk.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "comcap.h"

#ifndef SetFlag
#define SetFlag(_F, _SF)		((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)		((_F) &= ~(_SF))
#endif
#define CCP_MAX_COM_ID	32

//过滤设备和真实设备
static PDEVICE_OBJECT s_fltobj[CCP_MAX_COM_ID] = { 0 };
static PDEVICE_OBJECT s_nextobj[CCP_MAX_COM_ID] = { 0 };

//打开一个端口设备
PDEVICE_OBJECT ccpOpenCom(ULONG id, NTSTATUS *status)
{
	UNICODE_STRING name_str;
	static WCHAR name[32] = { 0 };
	PFILE_OBJECT fileobj = NULL;
	PDEVICE_OBJECT devobj = NULL;

	//输入字符串
	memset(name, 0, sizeof(WCHAR) * 32);
	RtlStringCchPrintfW(
		name, 32,
		L"\\Device\\Serial%d", id);
	RtlInitUnicodeString(&name_str, name);

	

	//打开设备对象
	*status = IoGetDeviceObjectPointer(&name_str, FILE_ALL_ACCESS, &fileobj, &devobj);

	if (*status == STATUS_SUCCESS)
	{
		ObDereferenceObject(fileobj);
		
		KdPrint(("%d is open\n", id));
	}
	
	return devobj;
}

NTSTATUS ccpAttachDevice(PDRIVER_OBJECT driver,
	PDEVICE_OBJECT oldobj,
	PDEVICE_OBJECT *fltobj,
	PDEVICE_OBJECT *next)
{
	NTSTATUS status;
	PDEVICE_OBJECT topdev = NULL;

	status = IoCreateDevice(driver,
		0,
		NULL,
		oldobj->DeviceType,
		0,
		FALSE,
		fltobj);
	if (status != STATUS_SUCCESS)
		return status;

	//拷贝重要标志位
	if (oldobj->Flags & DO_BUFFERED_IO)
		(*fltobj)->Flags |= DO_BUFFERED_IO;
	if (oldobj->Flags & DO_DIRECT_IO)
		(*fltobj)->Flags |= DO_DIRECT_IO;
	if (oldobj->Flags & DO_BUFFERED_IO)
		(*fltobj)->Characteristics |= DO_BUFFERED_IO;
	if (oldobj->Characteristics & FILE_DEVICE_SECURE_OPEN)
		oldobj->Characteristics |= FILE_DEVICE_SECURE_OPEN;
	(*fltobj)->Flags |= DO_POWER_PAGABLE;

	//绑定一个设备到另一个设备上
	topdev = IoAttachDeviceToDeviceStack(*fltobj, oldobj);
	if (topdev == NULL)
	{
		//绑定失败
		IoDeleteDevice(*fltobj);
		*fltobj = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	*next = topdev;

	//设置这个设备已经启动
	(*fltobj)->Flags = (*fltobj)->Flags & ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

void ccpAttachAllComs(PDRIVER_OBJECT driver)
{
	ULONG i;
	PDEVICE_OBJECT com_ob;
	NTSTATUS status;
	for (i = 0; i < CCP_MAX_COM_ID; ++i)
	{
		com_ob = ccpOpenCom(i, &status);
		if (com_ob == NULL)
		{
			continue;
		}
		//在这里的绑定，并不管绑定是否成功
		status = ccpAttachDevice(driver, com_ob, &s_fltobj[i], &s_nextobj[i]);
		if (NT_SUCCESS(status))
		{
			KdPrint(("%d is attach\n", i));
		}
	}
}

#define DELAY_ONE_MICROSECOND	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)
#define DELAY_ON_SECOND			(DELAY_ONE_MILLISECOND*1000)

void ccpUnload(PDRIVER_OBJECT drv)
{
	ULONG i;
	LARGE_INTEGER interval;

	//解除绑定
	for (i = 0; i < CCP_MAX_COM_ID; ++i)
	{
		if (s_nextobj[i] != NULL)
		{
			IoDetachDevice(s_nextobj[i]);
		}

		//睡眠5秒，等待所有irp处理结束
		interval.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
		KeDelayExecutionThread(KernelMode, FALSE, &interval);

		//删除设备
		for (i = 0; i < CCP_MAX_COM_ID; ++i)
		{
			if (s_fltobj[i] != NULL)
				IoDeleteDevice(s_fltobj[i]);
		}
	}

}

NTSTATUS ccpDispatch(PDEVICE_OBJECT device, PIRP pIrp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status;
	ULONG i, j;
	

	for (i = 0; i < CCP_MAX_COM_ID; ++i)
	{
		if (s_fltobj[i] == device)
		{
			

			//所有电源操作，全部直接放过
			if (irpsp->MajorFunction == IRP_MJ_POWER)
			{
				PoStartNextPowerIrp(pIrp);
				IoSkipCurrentIrpStackLocation(pIrp);
				return PoCallDriver(s_nextobj[i], pIrp);
			}

			//只过滤写请求
			if (irpsp->MajorFunction == IRP_MJ_WRITE)
			{

				KdPrint(("Enter Dispatch\n"));
				//如果是写，先获得长度
				ULONG len = irpsp->Parameters.Write.Length;
				PUCHAR buf = NULL;
				if (pIrp->MdlAddress != NULL)
				{
					buf = (PUCHAR)MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
				}
				else
					buf = (PUCHAR)pIrp->UserBuffer;
				if (buf == NULL)
					buf = (PUCHAR)pIrp->AssociatedIrp.SystemBuffer;
				KdPrint(("the len is: %d\n", len));

				KdPrint(("%s\n", buf));

				for (j = 0; j < len; ++j)
				{
					
					KdPrint(("comcap: Send Data: %2x\n", buf[j]));
				}
			}

			//这些请求直接下发执行
			IoSkipCurrentIrpStackLocation(pIrp);
			return IoCallDriver(s_nextobj[i], pIrp);
		}
	}

	//如果不在被绑定的设备中，就是有问题的，直接返回参数错误
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	size_t i;
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		driver->MajorFunction[i] = ccpDispatch;
	}

	//支持动态卸载
	driver->DriverUnload = ccpUnload;

	//绑定所有的串口
	ccpAttachAllComs(driver);

	return STATUS_SUCCESS;
}




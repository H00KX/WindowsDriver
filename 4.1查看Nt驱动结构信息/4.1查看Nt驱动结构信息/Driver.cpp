#include "Driver.h"

#pragma INITCODE
void Dump(IN PDRIVER_OBJECT pDriverObject)
{

}

NTSTATUS  CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{

	return STATUS_SUCCESS;
}

VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{

}

NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{

	return STATUS_SUCCESS;
}


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{

	return STATUS_SUCCESS;
}



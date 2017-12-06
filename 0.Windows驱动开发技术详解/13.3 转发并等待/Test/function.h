#ifndef FUNCTION_H 
#define FUNCTION_H

#include <setupapi.h>
#include <initguid.h>
#include "Ioctls.h"

HANDLE GetDeviceViaInterface(GUID* pGuid, DWORD instance);

DWORD ReadPort(HANDLE hDevice, DWORD port);
VOID WritePort(HANDLE hDevice, DWORD port, DWORD value);
VOID TestDriver(HANDLE hDevice);
#endif
#include "inthook.h"
struct DebuggerState11 DebuggerState;
struct InterruptHook11 InterruptHook[256];

VOID UNLOAD(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	DbgPrintEx(0, 0, "Entry\r\n");

	UNREFERENCED_PARAMETER(pRegPath);

	pDriver->DriverUnload = UNLOAD;
	
	callDPCFunctionForEachCpu(initIntHookForCurrentCPU_DPC, NULL, NULL, NULL);

	DbgPrintEx(0, 0, "Entry1\r\n");

	/*if (InterruptHook[3].hooked == 0) {
		return STATUS_UNSUCCESSFUL;
	}*/

	return STATUS_SUCCESS;

}
#include "inthook.h"

JUMPBACK Int1JumpBackLocation;
void initIntHookForCurrentCPU_DPC(IN KDPC* Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
{
	UNREFERENCED_PARAMETER(Dpc);
	UNREFERENCED_PARAMETER(DeferredContext);
	UNREFERENCED_PARAMETER(SystemArgument1);
	UNREFERENCED_PARAMETER(SystemArgument2);

	initIntHookForCurrentCPU();
}

BOOLEAN initIntHookForCurrentCPU()
{
	// hook int3
	BOOLEAN result = inthook_HookInterrupt(3, getCS() & 0xfff8, (ULONG_PTR)interrupt1_asmentry, &Int1JumpBackLocation);

	if (result == FALSE) {
		return FALSE;
	}

	// 如果是全局调试可以把dr7寄存器中的GD位设置为1
	// 这样可以确保在全局调试模式下，任何试图修改调试(dr0 - dr6)寄存器的行为都会被捕获并处理。

	// 启用Last Branch Record (LBR) 日志记录功能
	// LBR 是一种处理器功能，它可以追踪和记录最近发生的分支（如跳转和调用）信息。

	/*if (DebuggerState.storeLBR)
	{
		//DbgPrint("Enabling LBR logging. IA32_DEBUGCTL was %x\n", __readmsr(0x1d9));
		__writemsr(0x1d9, __readmsr(0x1d9) | 1);
		//DbgPrint("Enabling LBR logging. IA32_DEBUGCTL is  %x\n", __readmsr(0x1d9));
	}*/

	return TRUE;

}

BOOLEAN inthook_HookInterrupt(unsigned char intnr, int newCS, ULONG_PTR newEIP, PJUMPBACK jumpback)
{
	IDT idt;
	GetIDT(&idt);

	DbgPrintEx(0, 0, "cpu Number [%d], IDT Vector [%p]\r\n", cpunr(), idt.vector);

	// 非虚拟化情况下不能hook 0到31号中断 
	// 这些中断是非屏蔽中断(Non-Makeable Interrupts)
	//if (intnr < 32) {
	//	return FALSE;
	//}

	

	if (!InterruptHook[intnr].hooked)
	{
		InterruptHook[intnr].originalCS = idt.vector[intnr].wSelector;
		InterruptHook[intnr].originalEIP = (ULONG_PTR)idt.vector[intnr].wLowOffset + (ULONG_PTR)((ULONG_PTR)(idt.vector[intnr].wHighOffset) << 16ULL);
		InterruptHook[intnr].originalEIP |= (UINT64)((UINT64)(idt.vector[intnr].TopOffset) << 32ULL);
	}

	if (jumpback)
	{
		jumpback->cs = InterruptHook[intnr].originalCS;
		jumpback->eip = InterruptHook[intnr].originalEIP;
	}

	INT_VECTOR newVector;

	newVector.wHighOffset = (WORD)(newEIP >> 16);
	newVector.wLowOffset = (WORD)(newEIP);
	newVector.wSelector = newCS;
	newVector.bUnused = 0;
	newVector.bAccessFlags = idt.vector[intnr].bAccessFlags;

	newVector.TopOffset = newEIP >> 32;
	newVector.Reserved = 0;

	disableInterrupts();
	PHYSICAL_ADDRESS VectorPA = MmGetPhysicalAddress(&idt.vector[intnr]);
	DbgPrintEx(0, 0, "Virtual Address [%p], Physical Address [%llx]\r\n", &idt.vector[intnr], VectorPA.QuadPart);
	
	PVOID pmapped_mem = MmMapIoSpaceEx(VectorPA, sizeof(INT_VECTOR), PAGE_READWRITE);
	
	if (!pmapped_mem) {
		DbgPrintEx(0, 0, "MmMapIoSpaceEx Error\r\n");
		return FALSE;
	}
	DbgPrintEx(0, 0, "int%d hook, cpu Number: %d\r\n", intnr, cpunr());
	memcpy(pmapped_mem, &newVector, sizeof(INT_VECTOR));

	MmUnmapIoSpace(pmapped_mem, sizeof(INT_VECTOR));

	
	// idt.vector[intnr] = newVector;
	enableInterrupts();

	DbgPrintEx(0, 0, "int%d hook, Over \r\n", intnr);
	InterruptHook[intnr].hooked |= (1ULL << cpunr());

	return TRUE;
}

BOOLEAN inthook_UnHookInterrupt(unsigned char intnr)
{
	int cpuNumber = cpunr();

	if ((InterruptHook[intnr].hooked >> cpuNumber % 2) == 1)
	{
		INT_VECTOR newVector;

		newVector.wHighOffset = (WORD)((DWORD)(InterruptHook[intnr].originalEIP >> 16));
		newVector.wLowOffset = (WORD)InterruptHook[intnr].originalEIP;
		newVector.wSelector = (WORD)InterruptHook[intnr].originalCS;
		newVector.TopOffset = (InterruptHook[intnr].originalEIP >> 32);
		newVector.Reserved = 0;

		{
			IDT idt;
			GetIDT(&idt);


			newVector.bAccessFlags = idt.vector[intnr].bAccessFlags;

			disableInterrupts();

			PHYSICAL_ADDRESS VectorPA = MmGetPhysicalAddress(&idt.vector[intnr]);
			DbgPrintEx(0, 0, "Virtual Address [%p], Physical Address [%llx]\r\n", &idt.vector[intnr], VectorPA.QuadPart);

			PVOID pmapped_mem = MmMapIoSpaceEx(VectorPA, sizeof(INT_VECTOR), PAGE_READWRITE);
			if (!pmapped_mem) {
				DbgPrintEx(0, 0, "MmMapIoSpaceEx Error\r\n");
				return FALSE;
			}

			DbgPrintEx(0, 0, "int%d hook, cpu Number: %d\r\n", intnr, cpunr());
			memcpy(pmapped_mem, &newVector, sizeof(INT_VECTOR));

			MmUnmapIoSpace(pmapped_mem, sizeof(INT_VECTOR));

			//idt.vector[intnr] = newVector;
			enableInterrupts();
		}

		return TRUE;

	}

	return FALSE;
}

void callDPCFunctionForEachCpu(PKDEFERRED_ROUTINE dpcFunction, PVOID DeferredContext, PVOID SystemArg1, PVOID SystemArg2)
{
	ULONG cpucount = 0;
	KAFFINITY cpus = KeQueryActiveProcessors();

	while (cpus)
	{
		if (cpus % 2)
		{
			cpucount++;
		}

		cpus >>= 1;
	}

	PKDPC dpc = (PKDPC)ExAllocatePool(NonPagedPool, sizeof(KDPC) * cpucount);

	cpus = KeQueryActiveProcessors();
	CCHAR cpuNumber = 0;
	int dpcNumber = 0;

	while (cpus)
	{
		if (cpus % 2)
		{
			KeInitializeDpc(&dpc[dpcNumber], dpcFunction, DeferredContext);
			KeSetTargetProcessorDpc(&dpc[dpcNumber], cpuNumber);
			KeInsertQueueDpc(&dpc[dpcNumber], SystemArg1, SystemArg2);
			// KeFlushQueuedDpcs 放在循环内就排一个DPC就执行一个，适合短时间的DPC
			// 放在循环外就几乎同时开始执行，适合长时间或者需要同步的DPC
			KeFlushQueuedDpcs();

			dpcNumber++;

		}

		cpus /= 2;
		cpuNumber++;
	}
	if (dpc) ExFreePool(dpc);

}

BOOLEAN interrupt1_centry(UINT_PTR* stackpointer)
{
	DbgPrintEx(0, 0, "\n--------------------------- Hook Interrupt ---------------------------\n");
	DbgPrintEx(0, 0, "\n                   This Interrupt is hooked by Driver \n");
	DbgPrintEx(0, 0, "\n--------------------------- Hook Interrupt ---------------------------\n");

	IDT idt;
	GetIDT(&idt);

	UINT64 naddress = (UINT64)(idt.vector[1].wLowOffset) + ((UINT64)(idt.vector[1].wHighOffset) << 16ULL) + ((UINT64)(idt.vector[1].TopOffset) << 32ULL);

	stackpointer[si_errorcode] = (UINT_PTR)naddress; //the errorcode is used as address to call the original function if needed

	UINT_PTR before = getRSP();

	UINT_PTR currentDebugRegs[6];
	currentDebugRegs[0] = GetDr0Value();
	currentDebugRegs[1] = GetDr1Value();
	currentDebugRegs[2] = GetDr2Value();
	currentDebugRegs[3] = GetDr3Value();
	currentDebugRegs[4] = GetDr6DWORDValue();
	currentDebugRegs[5] = GetDr7DWORDValue();
	
	int handled = interrupt1_handler(stackpointer, currentDebugRegs);

	disableInterrupts(); //just making sure..	

	SetDr7GDValue(0); //make sure the GD bit is disabled

	if (handled) {
		SetDr6Value(0xffff0ff0);
	}

	disableInterrupts();

	if (handled == 2)
	{
		//DbgPrint("handled==2\n");		
		handled = 1; //epilogue = 1 Dr handler
	}

	return handled;
}

BOOLEAN interrupt1_handler(UINT_PTR* stackpointer, UINT_PTR* currentdebugregs)
{
	HANDLE CurrentProcessID = PsGetCurrentProcessId();
	//UINT_PTR originaldr6 = currentdebugregs[4];
	DebugReg6 _dr6 = *(DebugReg6*)&currentdebugregs[4];

	//UINT_PTR LBR_Stack[16];

	if (DebuggerState.isDebugging)
	{
		if (CurrentProcessID == (HANDLE)(UINT_PTR)DebuggerState.debuggedProcessID)
		{
			//UINT_PTR originaldebugregs[6];
			//UINT64 oldDR7 = GetDr7DWORDValue();
			// IF 标志用于控制外部中断（除了非屏蔽中断 NMI）的响应。
			// 当 IF 设置为 1 时，处理器可以响应可屏蔽的外部中断。
			// 当 IF 清零为 0 时，这些中断被禁用。这通常在关键代码段中发生，其中中断可能会导致问题或者当内核需要原子地执行某些操作时。
			if ((((PEFLAGS)&stackpointer[si_eflags])->IF == 0) || (KeGetCurrentIrql() != PASSIVE_LEVEL))
			{
				//if (!KernelCodeStepping) // 不单步调试
				//{
				//	((PEFLAGS)&stackpointer[si_eflags])->TF = 0; //just give up stepping
				////	DbgPrint("Quitting this");
				//}
				//else // 单步调试
				//{
				//	//	DbgPrint("Stepping until valid\n");
				//	((PEFLAGS)&stackpointer[si_eflags])->TF = 1; //keep going until a valid state
				//	DebuggerState.isSteppingTillClear = TRUE; //Just in case a taskswitch happens right after enabling passive level with interrupts
				//}

				// 避免单步无法取消
				//((PEFLAGS)&stackpointer[si_eflags])->RF = 1;
				SetDr6Value(0xffff0ff0);
				return TRUE;

			}

			// DebuggerState.isSteppingTillClear = FALSE;

			SetDr7DWORDValue(0x400);
			SetDr0Value(0);
			SetDr1Value(0);
			SetDr2Value(0);
			SetDr3Value(0);
			SetDr6Value(0xffff0ff0);

			//BOOL NeedsToGrowStackList = FALSE;
			//PSavedStack SelectedStackEntry = NULL;

			{
				int rs = 1;

				/*if (SelectedStackEntry == NULL)
					rs = breakpointHandler_kernel(stackpointer, currentdebugregs, LBR_Stack);
				else
					rs = breakpointHandler_kernel((UINT_PTR*)(SelectedStackEntry->stacksnapshot), currentdebugregs, LBR_Stack);*/

				disableInterrupts();

				SetDr0Value(currentdebugregs[0]);
				SetDr1Value(currentdebugregs[1]);
				SetDr2Value(currentdebugregs[2]);
				SetDr3Value(currentdebugregs[3]);
				SetDr6Value(currentdebugregs[4]);

				if ((currentdebugregs[5] >> 13) & 1)
				{
					//	DbgPrint("WTF? GD is 1 in currentdebugregs[5]: %llx\n", currentdebugregs[5]);
				}
				else
					SetDr7Value(*(DebugReg7*)&currentdebugregs[5]);

				return rs;
			}

		}
	}

	return FALSE;
}

//int breakpointHandler_kernel(UINT_PTR* stackpointer, UINT_PTR* currentdebugregs, UINT_PTR* LBR_Stack)
//{
//	
//	NTSTATUS r = STATUS_UNSUCCESSFUL;
//	int handled = 0; //0 means let the OS handle it
//	LARGE_INTEGER timeout;
//	timeout.QuadPart = -100000;
//
//	if (KeGetCurrentIrql() != 0) {
//		return 1;
//	}
//
//	if ((stackpointer[si_cs] & 3) == 0)
//	{
//		DbgPrint("Going to wait in a kernelmode routine\n");
//	}
//
//	while (r != STATUS_SUCCESS)
//	{
//		r = KeWaitForSingleObject(&debugger_event_CanBreak, Executive, KernelMode, FALSE, NULL);
//		//check r and handle specific events
//
//		//DbgPrint("Woke up. r=%x\n",r);
//
//	}
//
//}

#pragma once
#include "helpfunction.h"

typedef enum { 
	si_eax = 0, 
	si_ebx = 1, 
	si_ecx = 2, 
	si_edx = 3, 
	si_esi = 4, 
	si_edi = 5, 
	si_ebp = 6, 
	si_stack_esp = 7, 
	si_r8 = 8, 
	si_r9 = 9, 
	si_r10 = 10, 
	si_r11 = 11, 
	si_r12 = 12, 
	si_r13 = 13, 
	si_r14 = 14,
	si_r15 = 15, 
	si_es = 16, 
	si_ds = 17, 
	si_stack_ss = 18, 
	si_xmm = 19, 
	si_errorcode = 19 + (4096 / 8) + (512 / 8), 
	si_eip = 20 + (4096 / 8) + (512 / 8), 
	si_cs = 21 + (4096 / 8) + (512 / 8), 
	si_eflags = 22 + (4096 / 8) + (512 / 8), 
	si_esp = 23 + (4096 / 8) + (512 / 8), 
	si_ss = 24 + (4096 / 8) + (512 / 8) } stackindex;

extern JUMPBACK Int1JumpBackLocation;

void initIntHookForCurrentCPU_DPC(IN KDPC* Dpc, IN PVOID  DeferredContext, IN PVOID  SystemArgument1, IN PVOID  SystemArgument2);

BOOLEAN initIntHookForCurrentCPU();

BOOLEAN inthook_HookInterrupt(unsigned char intnr, int newCS, ULONG_PTR newEIP, PJUMPBACK jumpback);

BOOLEAN inthook_UnHookInterrupt(unsigned char intnr);

// IDT 每个逻辑处理器单独分离的 所以 hook 要对每个逻辑处理器做
void callDPCFunctionForEachCpu(PKDEFERRED_ROUTINE dpcFunction, PVOID DeferredContext, PVOID SystemArg1, PVOID SystemArg2);

//code segment 8 has a 32-bit stackpointer
BOOLEAN interrupt1_centry(UINT_PTR* stackpointer);

BOOLEAN interrupt1_handler(UINT_PTR* stackpointer, UINT_PTR* currentdebugregs);

//int breakpointHandler_kernel(UINT_PTR* stackpointer, UINT_PTR* currentdebugregs, UINT_PTR* LBR_Stack);
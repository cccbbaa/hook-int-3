#pragma once
#include <windef.h>

typedef enum { bt_OnInstruction = 0, bt_OnWrites = 1, bt_OnIOAccess = 2, bt_OnReadsAndWrites = 3 } BreakType;
typedef enum { bl_1byte = 0, bl_2byte = 1, bl_8byte = 2/*Only when in 64-bit*/, bl_4byte = 3 } BreakLength;

volatile struct DebuggerState11
{
	BOOL		isDebugging;		//TRUE if a process is currently being debugged
	BOOL		stoppingTheDebugger;
	DWORD		debuggedProcessID;	//The processID that is currently debugger
	struct {
		BOOL		active;
		UINT_PTR	address;		//Up to 4 addresses to break on
		BreakType	breakType;		//What type of breakpoint for each seperate address
		BreakLength breakLength;	//How many bytes does this breakpoint look at
	} breakpoint[4];


	//while debugging:
	UINT_PTR* LastStackPointer;
	UINT_PTR* LastRealDebugRegisters;
	HANDLE LastThreadID;
	BOOL handledlastevent;

	//BOOL storeLBR;
	//int storeLBR_max;
	//UINT_PTR *LastLBRStack;

	char b[1];

	//volatile BYTE DECLSPEC_ALIGN(16) fxstate[512];

	BOOL isSteppingTillClear; //when set the user has entered single stepping mode. This is a one thread only thing, so when it's active and another single step happens, discard it

};// DebuggerState;

volatile struct InterruptHook11
{
	ULONG_PTR hooked; // 按位运算来，表示对应cpu编号的对应中断是否被hook
	WORD originalCS;
	ULONG_PTR originalEIP;
};// InterruptHook[256];

extern struct InterruptHook11 InterruptHook[256];
extern struct DebuggerState11 DebuggerState;

#pragma pack(1) //allignment of 1 byte
typedef struct tagINT_VECTOR
{
	WORD	wLowOffset;
	WORD	wSelector;
	BYTE	bUnused;
	BYTE    bAccessFlags;

	/*
	unsigned gatetype  : 3; //101=Task, 110=interrupt, 111=trap
	unsigned gatesize  : 1; //1=32bit, 0=16bit
	unsigned zero      : 1;
	unsigned DPL       : 2;
	unsigned P         : 1;
	*/
	WORD	wHighOffset;
#ifdef AMD64
	DWORD	TopOffset;
	DWORD	Reserved;
#endif
} INT_VECTOR, * PINT_VECTOR;
#pragma pack()

#pragma pack(2) //allignemnt of 2 byte
typedef struct tagIDT
{
	WORD wLimit;
	PINT_VECTOR vector;
} IDT, * PIDT;
#pragma pack()

typedef
#pragma pack(1) //allignemnt of 1 byte
struct
{
	UINT64 eip;
	WORD cs;
} JUMPBACK, * PJUMPBACK;
#pragma pack()

typedef struct tagDebugReg7
{
	unsigned L0 : 1; //			0
	unsigned G0 : 1; //			1
	unsigned L1 : 1; //			2
	unsigned G1 : 1; //			3
	unsigned L2 : 1; //			4
	unsigned G2 : 1; //			5
	unsigned L3 : 1; //			6
	unsigned G3 : 1; //			7
	unsigned GL : 1; //			8
	unsigned GE : 1; //			9
	unsigned undefined_1 : 1; //1       10
	unsigned RTM : 1; //        11
	unsigned undefined_0 : 1; //0       12
	unsigned GD : 1; //		   13
	unsigned undefined2 : 2; // 00 
	unsigned RW0 : 2;
	unsigned LEN0 : 2;
	unsigned RW1 : 2;
	unsigned LEN1 : 2;
	unsigned RW2 : 2;
	unsigned LEN2 : 2;
	unsigned RW3 : 2;
	unsigned LEN3 : 2;
#ifdef AMD64
	unsigned undefined3 : 8;
	unsigned undefined4 : 8;
	unsigned undefined5 : 8;
	unsigned undefined6 : 8;
#endif
} DebugReg7;

typedef struct DebugReg6
{
	unsigned B0 : 1;
	unsigned B1 : 1;
	unsigned B2 : 1;
	unsigned B3 : 1;
	unsigned undefined1 : 9; // 011111111
	unsigned BD : 1;
	unsigned BS : 1;
	unsigned BT : 1;
	unsigned RTM : 1; //0=triggered
	unsigned undefined2 : 15; // 111111111111111
#ifdef AMD64
	unsigned undefined3 : 8;
	unsigned undefined4 : 8;
	unsigned undefined5 : 8;
	unsigned undefined6 : 8;
#endif
} DebugReg6;

typedef struct
{
	unsigned CF : 1; // 0
	unsigned reserved1 : 1; // 1
	unsigned PF : 1; // 2
	unsigned reserved2 : 1; // 3
	unsigned AF : 1; // 4
	unsigned reserved3 : 1; // 5
	unsigned ZF : 1; // 6
	unsigned SF : 1; // 7
	unsigned TF : 1; // 8
	unsigned IF : 1; // 9
	unsigned DF : 1; // 10
	unsigned OF : 1; // 11
	unsigned IOPL : 2; // 12+13
	unsigned NT : 1; // 14
	unsigned reserved4 : 1; // 15
	unsigned RF : 1; // 16
	unsigned VM : 1; // 17
	unsigned AC : 1; // 18
	unsigned VIF : 1; // 19
	unsigned VIP : 1; // 20
	unsigned ID : 1; // 21
	unsigned reserved5 : 10; // 22-31
#ifdef AMD64
	unsigned reserved6 : 8;
	unsigned reserved7 : 8;
	unsigned reserved8 : 8;
	unsigned reserved9 : 8;
#endif
} EFLAGS, * PEFLAGS;

typedef struct _SavedStack
{
	BOOL inuse;
	INT64 stacksnapshot[600];
} SavedStack, * PSavedStack;
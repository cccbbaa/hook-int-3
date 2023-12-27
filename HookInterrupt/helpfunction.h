#pragma 
#include <ntifs.h>
#include <intrin.h>

#include "helpstruct.h"


EXTERN_C UINT64 getRSP(void);
EXTERN_C WORD getCS(void);
EXTERN_C void interrupt1_asmentry(void);

inline void GetIDT(PIDT pIdt)
{
	__sidt(pIdt);
}

inline void enableInterrupts(void)
{
	_enable();
}

inline void disableInterrupts(void)
{
	_disable();
}
// 获取当前执行线程的 CPU 编号
inline int cpunr(void)
{
	int x[4];
	__cpuid(&x[0], 1);

	return (x[1] >> 24) + 1;
}

inline UINT_PTR GetDr0Value(void)
{
	return __readdr(0);
}


inline void SetDr0Value(UINT_PTR value)
{
	__writedr(0, value);
}

inline UINT_PTR GetDr1Value(void)
{
	return __readdr(1);
}

inline void SetDr1Value(UINT_PTR value)
{
	__writedr(1, value);
}

inline UINT_PTR GetDr2Value(void)
{
	return __readdr(2);
}

inline void SetDr2Value(UINT_PTR value)
{
	__writedr(2, value);
}

inline UINT_PTR GetDr3Value(void)
{
	return __readdr(3);
}

inline void SetDr3Value(UINT_PTR value)
{
	__writedr(3, value);
}

inline UINT_PTR GetDr6DWORDValue(void)
{
	return __readdr(6);
}

inline UINT_PTR GetDr7DWORDValue(void)
{
	return __readdr(7);
}

inline DebugReg7 GetDr7Value(void)
{
	UINT_PTR temp = GetDr7DWORDValue();
	return *(DebugReg7*)&temp;
}

inline void SetDr7DWORDValue(UINT_PTR value)
{
	__writedr(7, value);
}

inline void SetDr7Value(DebugReg7 value)
{
	UINT_PTR temp = *(UINT_PTR*)&value;
	__writedr(7, temp);
}

inline void SetDr7GDValue(int state)
{
	DebugReg7 _dr7 = GetDr7Value();
	_dr7.GD = state; //usually 1
	SetDr7Value(_dr7);
}

inline void SetDr6Value(UINT_PTR value)
{
	__writedr(6, value);
}

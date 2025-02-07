#include "stdafx.h"
#include "undname.h"

#include "rtlframe.h"

typedef RTL_FRAME<DATA_BLOB> AFRAME;

static void* __cdecl fAlloc(ULONG cb)
{
	if (DATA_BLOB* prf = AFRAME::get())
	{
		if (cb > prf->cbData)
		{
			return 0;
		}
		prf->cbData -= cb;
		PVOID pv = prf->pbData;
		prf->pbData += cb;
		return pv;
	}

	return 0;
}

static void __cdecl fFree(void*)
{
}

EXTERN_C
_CRTIMP
PSTR __cdecl __unDNameEx
(
	PSTR buffer,
	PCSTR mangled,
	DWORD cb,
	void* (__cdecl* memget)(DWORD),
	void(__cdecl* memfree)(void*),
	PSTR(__cdecl* GetParameter)(long i),
	DWORD flags
);

EXTERN_C PVOID _imp____unDNameEx = 0;

PSTR __cdecl GetParameter(long /*i*/)
{
	return const_cast<PSTR>("");
}

static PSTR _unDName(PCSTR mangled, PSTR buffer, DWORD cb, DWORD flags)
{
	if (_imp____unDNameEx)
	{
	__ok:
		AFRAME af;
		af.cbData = 32 * PAGE_SIZE;
		af.pbData = (PUCHAR)alloca(32 * PAGE_SIZE);

		return __unDNameEx(buffer, mangled, cb, fAlloc, fFree, GetParameter, flags);
	}

	if (HMODULE hmod = LoadLibraryW(L"msvcrt.dll"))
	{
		if (_imp____unDNameEx = GetProcAddress(hmod, "__unDNameEx"))
		{
			goto __ok;
		}
	}

	return 0;
}

PCSTR unDNameEx(PCSTR mangled, PSTR buffer, DWORD cb, DWORD flags)
{
	if ('?' != *mangled)
	{
		return mangled;
	}
	PSTR sz = _unDName(mangled, buffer, cb, flags);
	return sz ? sz : mangled;
}
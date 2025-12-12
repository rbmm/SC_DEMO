#include "stdafx.h"

_NT_BEGIN

ULONG GetZwCount(HMODULE hmod, PULONG AddressOfNames, DWORD NumberOfNames)
{
	ULONG n = 0;
	do
	{
		PCSTR name = RtlOffsetToPointer(hmod, *AddressOfNames++);
		if ('Z' == *name && 'w' == name[1])
		{
			n++;
		}
	} while (--NumberOfNames);

	return n;
}

EXTERN_C_START
HMODULE _S_hnt;
PULONG _S_Rva;
EXTERN_C_END

struct RAH
{
	union {
		ULONG rva;
		ULONG index;
	};
	ULONG hash;

	inline static RAH* _S_Table;
	inline static ULONG _S_n;

	static int __cdecl compare(const void* p, const void* q)
	{
		if (reinterpret_cast<const RAH*>(p)->rva < reinterpret_cast<const RAH*>(q)->rva) return -1;
		if (reinterpret_cast<const RAH*>(p)->rva > reinterpret_cast<const RAH*>(q)->rva) return +1;
		return 0;
	}

	static int __cdecl compare2(const void* p, const void* q)
	{
		if (reinterpret_cast<const RAH*>(p)->hash < reinterpret_cast<const RAH*>(q)->hash) return -1;
		if (reinterpret_cast<const RAH*>(p)->hash > reinterpret_cast<const RAH*>(q)->hash) return +1;
		__debugbreak();
		return 0;
	}

	static int GetIndex(ULONG Hash)
	{
		ULONG a = 0, o, b = _S_n;
		RAH* p = _S_Table;

		do
		{
			ULONG h = p[o = (a + b) >> 1].hash;
			if (h == Hash)
			{
				return p[o].index;
			}

			if (Hash < h) b = o; else a = o + 1;
		} while (a < b);

		return -1;
	}

	//static int GetIndex(PCSTR name)
	//{
	//	name += 2;
	//	ULONG hash = RtlComputeCrc32(5, name, (ULONG)strlen(name));
	//	return GetIndex(LOWORD(hash) ^ HIWORD(hash));
	//}
};

void FreeZwSupport()
{
	if (RAH::_S_Table)
	{
		delete[] RAH::_S_Table;
		RAH::_S_Table = 0;
		RAH::_S_n = 0;
	}

	if (_S_Rva)
	{
		delete[] _S_Rva;
		_S_Rva = 0;
	}
}

BOOLEAN IsZwSupportInit()
{
	return 0 != RAH::_S_Table;
}

ULONG GetSysCallRet(ULONG rva)
{
	PBYTE pb = (PBYTE)_S_hnt + rva + 0x20;

	ULONG n = 0x20;
	do 
	{
		if (0xc3 == *--pb)
		{
			if (0x05 == *--pb)
			{
				if (0x0f == *--pb)
				{
					return RtlPointerToOffset(_S_hnt, pb);
				}

				++pb;
			}
			++pb;
		}
	} while (--n);

	return 0;
}

BOOLEAN InitZwSupport()
{
	if (HMODULE hmod = GetModuleHandleW(L"ntdll"))
	{
		_S_hnt = hmod;

		ULONG s;
		if (PIMAGE_EXPORT_DIRECTORY pied = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
			hmod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &s))
		{
			if (ULONG NumberOfNames = pied->NumberOfNames)
			{
				PULONG AddressOfNames = (PULONG)RtlOffsetToPointer(hmod, pied->AddressOfNames);
				if (ULONG n = GetZwCount(hmod, AddressOfNames, NumberOfNames))
				{
					PULONG AddressOfFunctions = (PULONG)RtlOffsetToPointer(hmod, pied->AddressOfFunctions);
					PUSHORT AddressOfNameOrdinals = (PUSHORT)RtlOffsetToPointer(hmod, pied->AddressOfNameOrdinals);
					ULONG NumberOfFunctions = pied->NumberOfFunctions;

					if (RAH* p = new RAH[n])
					{
						RAH* q = p;
						do
						{
							USHORT o = *AddressOfNameOrdinals++;
							if (o < NumberOfFunctions)
							{
								PCSTR name = RtlOffsetToPointer(hmod, *AddressOfNames++);
								if ('Z' == *name++ && 'w' == *name++)
								{
									p->rva = AddressOfFunctions[o];//

									ULONG hash = RtlComputeCrc32(5, name, (ULONG)strlen(name));

									p++->hash = hash = LOWORD(hash) ^ HIWORD(hash);

									//DbgPrint("API Zw%hs\n", name, name);
									//DbgPrint("Zw%hs @%u NONAME\n", name, hash);
								}
							}
							else
							{
								delete q;
								return FALSE;
							}
						} while (--NumberOfNames);

						qsort(q, n, sizeof(RAH), RAH::compare);

						PULONG pRVA = new ULONG[n];

						pRVA += n;

						ULONG m = n;
						do
						{
							*--pRVA = GetSysCallRet((--p)->rva);
							p->index = --m;
						} while (m);

						qsort(q, n, sizeof(RAH), RAH::compare2);

						RAH::_S_Table = p;
						RAH::_S_n = n;
						_S_Rva = pRVA;

						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

#include <delayimp.h>

LONG GuardedWrite(PEXCEPTION_RECORD ExceptionRecord, PVOID addr)
{
	if (STATUS_ACCESS_VIOLATION == ExceptionRecord->ExceptionCode &&
		1 < ExceptionRecord->NumberParameters &&
		1 == ExceptionRecord->ExceptionInformation[0] &&
		(ULONG_PTR)addr == ExceptionRecord->ExceptionInformation[1])
	{
		ULONG op;
		if (VirtualProtect(addr, sizeof(PVOID), PAGE_READWRITE, &op))
		{
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

EXTERN_C extern UCHAR ZwFuncs[];

EXTERN_C PVOID NTAPI __delayLoadHelper2(PImgDelayDescr pidd, PImgThunkData ppfnIATEntry)
{
	PVOID* phmod = (PVOID*)RtlOffsetToPointer(&__ImageBase, pidd->rvaHmod);

	PVOID hmod = *phmod;

	if (!hmod)
	{
		UNICODE_STRING us;

		PCSTR pcszDLLName = RtlOffsetToPointer(&__ImageBase, pidd->rvaDLLName);
		if (strcmp("api-ms-win-core-nt-l1-1-0.dll", pcszDLLName))
		{
			if (RtlCreateUnicodeStringFromAsciiz(&us, pcszDLLName))
			{
				if (0 <= LdrLoadDll(0, 0, &us, (HMODULE*)&hmod))
				{
					if (PVOID hmod2 = InterlockedCompareExchangePointer(phmod, hmod, 0))
					{
						LdrUnloadDll((HMODULE)hmod);
						hmod = hmod2;
					}
				}

				RtlFreeUnicodeString(&us);
			}

			if (!hmod)
			{
				__debugbreak();
			}

		}
		else
		{
			*phmod = hmod = INVALID_HANDLE_VALUE;
		}
	}

	PCImgThunkData ppfnINTEntry = (PCImgThunkData)RtlOffsetToPointer(ppfnIATEntry,
		(INT_PTR)pidd->rvaINT - (INT_PTR)pidd->rvaIAT);

	ANSI_STRING as, * ProcedureName = 0;
	ULONG Ordinal = 0;

	if (IMAGE_SNAP_BY_ORDINAL(ppfnINTEntry->u1.Ordinal))
	{
		Ordinal = IMAGE_ORDINAL(ppfnINTEntry->u1.Ordinal);
	}
	else
	{
		RtlInitString(ProcedureName = &as,
			(PCSTR)((PIMAGE_IMPORT_BY_NAME)RtlOffsetToPointer(&__ImageBase, ppfnINTEntry->u1.AddressOfData))->Name);
	}

	PVOID pfn = 0;
	if (INVALID_HANDLE_VALUE == hmod)
	{
		if (Ordinal)
		{
			int i = RAH::GetIndex(Ordinal);
			if (0 > i)
			{
				__debugbreak();
			}
			pfn = &ZwFuncs[i * 9];
		}
		else
		{
			__debugbreak();
		}
	}
	else if (0 > LdrGetProcedureAddress((HMODULE)hmod, ProcedureName, Ordinal, &pfn))
	{
		__debugbreak();
	}

	__try {
		ppfnIATEntry->u1.Function = (ULONG_PTR)pfn;
	}
	__except (GuardedWrite(GetExceptionInformation()->ExceptionRecord, &ppfnIATEntry->u1.Function)) {
		__debugbreak();
	}

	return pfn;
}

_NT_END
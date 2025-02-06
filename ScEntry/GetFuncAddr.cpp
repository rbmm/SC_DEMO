#include "stdafx.h"

#pragma intrinsic(strcmp, strlen)

//#define _PRINT_CPP_NAMES_
#include "asmfunc.h"

PVOID GetNtBase()
{
	CPP_FUNCTION;

	return CONTAINING_RECORD(NtCurrentTeb()->ProcessEnvironmentBlock->Ldr->InInitializationOrderModuleList.Flink,
		_LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks)->DllBase;
}

ULONG HashString(PCSTR lpsz, ULONG hash = 0)
{
	while (char c = *lpsz++) hash = hash * 33 ^ c;
	return hash;
}

PVOID __fastcall get_hmod(PCWSTR pwz)
{
	CPP_FUNCTION;

	if (*pwz)
	{
		PVOID hmod;
		UNICODE_STRING us;
		RtlInitUnicodeString(&us, pwz);
		return 0 > LdrLoadDll(0, 0, &us, &hmod) ? 0 : hmod;
	}

	return GetNtBase();
}

ULONG StrToInt(PCSTR psz)
{
	ULONG i = 0;
	while (ULONG c = *psz++)
	{
		if ((c -= '0') > 9) return 0;
		i = 10 * i + c;
	}
	return i;
}

PVOID __fastcall GetFuncAddressEx(PIMAGE_DOS_HEADER pidh, ULONG hash)
{
	CPP_FUNCTION;

	PIMAGE_NT_HEADERS pinth = (PIMAGE_NT_HEADERS)RtlOffsetToPointer(pidh, pidh->e_lfanew);

	PIMAGE_EXPORT_DIRECTORY pied = (PIMAGE_EXPORT_DIRECTORY)RtlOffsetToPointer(pidh, 
		pinth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	DWORD Ordinal;

	if (0x80000000 == (hash & 0xFFFF0000))
	{
		Ordinal = IMAGE_ORDINAL(hash) - pied->Base;

	__index:

		if (Ordinal < pied->NumberOfFunctions)
		{
			PVOID fn = RtlOffsetToPointer(pidh, ((PDWORD)RtlOffsetToPointer(pidh, pied->AddressOfFunctions))[Ordinal]);

			ULONG size = pinth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			
			ULONG_PTR u = (ULONG_PTR)fn - (ULONG_PTR)pied;

			if (u >= size)
			{
				return fn;
			}

			size -= (ULONG)u;

			PCSTR pc = (PCSTR)fn, name = 0;
			do
			{
				switch (*pc++)
				{
				case '.':
					name = pc;
					break;
				case 0:
					if (name)
					{
						ULONG BytesInMultiByteString = RtlPointerToOffset(fn, name);
						ULONG BytesInUnicodeString = 0;

						if (0 <= RtlMultiByteToUnicodeSize(&BytesInUnicodeString, (PSTR)fn, BytesInMultiByteString))
						{
							PWCH UnicodeString = (PWCH)alloca(BytesInUnicodeString + 4 * sizeof(WCHAR));

							if (0 <= RtlMultiByteToUnicodeN(UnicodeString, BytesInUnicodeString, 
								&BytesInUnicodeString, (PSTR)fn, BytesInMultiByteString))
							{
								UNICODE_STRING us = {
									(USHORT)BytesInUnicodeString + 3 * sizeof(WCHAR),
									(USHORT)BytesInUnicodeString + 4 * sizeof(WCHAR),
									UnicodeString
								};

								(PBYTE&)UnicodeString += BytesInUnicodeString;

								*UnicodeString++ = 'D';
								*UnicodeString++ = 'L';
								*UnicodeString++ = 'L';
								*UnicodeString++ = 0;

								Ordinal = 0;
								ANSI_STRING as, * ProcedureName = 0;
								if ('#' == *name)
								{
									Ordinal = StrToInt(name + 1);

									if ((ULONG)(Ordinal - 1) > MAXWORD)
									{
										__debugbreak();
										return 0;
									}
								}
								else
								{
									RtlInitAnsiString(ProcedureName = &as, name);
								}

								PVOID hmod;
								if (0 <= LdrLoadDll(0, 0, &us, &hmod) &&
									0 <= LdrGetProcedureAddress(hmod, ProcedureName, Ordinal, &fn))
								{
									return fn;
								}
							}
						}
					}

					__debugbreak();
					return 0;
				}
			} while (--size);
		}
	}
	else if (DWORD NumberOfNames = pied->NumberOfNames)
	{
		PULONG AddressOfNames = (PULONG)RtlOffsetToPointer(pidh, pied->AddressOfNames);
		PWORD AddressOfNameOrdinals = (PWORD)RtlOffsetToPointer(pidh, pied->AddressOfNameOrdinals);
		do
		{
			Ordinal = *AddressOfNameOrdinals++;
			if (HashString(RtlOffsetToPointer(pidh, *AddressOfNames++)) == hash)
			{
				goto __index;
			}

		} while (--NumberOfNames);
	}

	__debugbreak();
	return 0;
}
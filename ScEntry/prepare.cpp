#include "stdafx.h"

//#define _PRINT_CPP_NAMES_
#include "asmfunc.h"

void WINAPI epASM()ASM_FUNCTION;

#pragma code_seg(".text$nm")

void* sc_end()
{
	return sc_end;
}

#pragma code_seg(".text$zz")

PVOID GetNtBase();
PVOID __fastcall GetFuncAddressEx(PIMAGE_DOS_HEADER pidh, PCSTR ProcedureName);

// 64: ?ScEntry@@YAXPEAU_PEB@@@Z
// 32: ?ScEntry@@YGXPAU_PEB@@@Z

void WINAPI ScEntry(PEB* peb)
{
	CPP_FUNCTION;

	union {
		PVOID pv;

		void (NTAPI * RtlExitUserProcess)(ULONG ExitCode);

		NTSTATUS (NTAPI * LdrLoadDll)(
				_In_opt_ PWSTR DllPath,
				_In_opt_ PULONG DllCharacteristics,
				_In_ PUNICODE_STRING DllName,
				_Out_ PVOID* DllHandle
			);

		NTSTATUS (NTAPI * LdrGetProcedureAddress)(
				_In_ PVOID DllHandle,
				_In_opt_ PANSI_STRING ProcedureName,
				_In_opt_ ULONG ProcedureNumber,
				_Out_ PVOID* ProcedureAddress
			);

		NTSTATUS (NTAPI * LdrUnloadDll)(
				_In_ PVOID DllHandle
			);

		VOID (NTAPI * RtlInitUnicodeString)(
				_Out_ PUNICODE_STRING DestinationString,
				_In_opt_z_ __drv_aliasesMem PCWSTR SourceString
			);

		NTSTATUS (NTAPI* PrepareSC)(
			_In_ PVOID Base, _In_ ULONG cb, _In_ PVOID ImageBase
			);
	};

	PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)GetNtBase();

#define RAC(fn, ...) (pv = GetFuncAddressEx(pidh, #fn), fn)(__VA_ARGS__)
	
	UNICODE_STRING DllName;
	RAC(RtlInitUnicodeString, &DllName, L"prepare.dll");

	PVOID hmod;
	NTSTATUS status;

	if (0 <= (status = RAC(LdrLoadDll, 0, 0, &DllName, &hmod)))
	{
		if (0 <= (status = RAC(LdrGetProcedureAddress, hmod, 0, 1, &pv)))
		{
			status = PrepareSC(epASM, RtlPointerToOffset(epASM, sc_end()), &__ImageBase);
		}

		pv = GetFuncAddressEx(pidh, "LdrUnloadDll");
		RAC(LdrUnloadDll, hmod);
	}

	if (peb->BeingDebugged)
	{
		epASM();
	}

	RAC(RtlExitUserProcess, status);
}

#include "stdafx.h"

_NT_BEGIN

#include "../DelayImp/DirectSysCall.h"

NTSTATUS RedirectContext(PVOID BaseAddress)
{
	CONTEXT ctx = {};
	RtlCaptureContext(&ctx);

	_PEB* peb = RtlGetCurrentPeb(); // 2 parameter

	PVOID ImageBaseAddress = peb->ImageBaseAddress;

	if (PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(ImageBaseAddress))
	{
		(ULONG_PTR&)ImageBaseAddress += pinth->OptionalHeader.AddressOfEntryPoint; // 1 parameter

		if (PVOID Rip = GetProcAddress(GetModuleHandleW(L"ntdll"), "RtlUserThreadStart"))
		{
			PNT_TIB Tib = reinterpret_cast<PNT_TIB>(NtCurrentTeb());
			union {
				PCONTEXT pctx;
				ULONG_PTR StackBase;
			};
			for (StackBase = (ULONG_PTR)Tib->StackBase - sizeof(CONTEXT);
				&ctx < pctx;
				StackBase -= __alignof(CONTEXT))
			{
				if (pctx->Rcx == (ULONG_PTR)ImageBaseAddress &&
					pctx->Rdx == (ULONG_PTR)peb &&
					pctx->Rip == (ULONG_PTR)Rip &&
					pctx->SegCs == ctx.SegCs &&
					pctx->SegSs == ctx.SegSs)
				{
					pctx->Rcx = (ULONG_PTR)BaseAddress;

					//LdrSetDllManifestProber(MyDllManifestProber, 0, 0);
					return STATUS_SUCCESS;
				}
			}
		}
	}

	return STATUS_NOT_FOUND;
}

PCWSTR GetShellName(PCWSTR* ppc)
{
	if (PWSTR psz = wcschr(GetCommandLineW(), '*'))
	{
		if (PWSTR pc = wcschr(++psz, '*'))
		{
			*pc++ = 0;
			*ppc = pc;
			return psz;
		}
	}

	return L"sc.bin";
}

NTSTATUS OnProcessAttach()
{
	NTSTATUS status = STATUS_INTERNAL_ERROR;

	if (InitZwSupport())
	{
		status = STATUS_INVALID_PARAMETER_1;
		PCWSTR pc = 0;
		if (PCWSTR psz = GetShellName(&pc))
		{
			UNICODE_STRING ObjectName;
			OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName };
			if (0 <= (status = RtlDosPathNameToNtPathName_U_WithStatus(psz, &ObjectName, 0, 0)))
			{
				if (pc) wcscpy(GetCommandLineW(), pc);
				HANDLE hFile;
				IO_STATUS_BLOCK iosb;
				status = ZwOpenFile(&hFile, FILE_GENERIC_READ, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
				RtlFreeUnicodeString(&ObjectName);
				if (0 <= status)
				{
					FILE_STANDARD_INFORMATION fsi;
					if (0 <= (status = ZwQueryInformationFile(hFile, &iosb, &fsi, sizeof(fsi), FileStandardInformation)))
					{
						if (!fsi.EndOfFile.QuadPart || fsi.EndOfFile.HighPart)
						{
							status = STATUS_FILE_TOO_LARGE;
						}
						else
						{
							PVOID BaseAddress = 0;
							SIZE_T RegionSize = fsi.EndOfFile.QuadPart;

							if (0 <= (status = ZwAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0,
								&RegionSize, MEM_TOP_DOWN | MEM_COMMIT, PAGE_READWRITE)))
							{
								if (0 > (status = ZwReadFile(hFile, 0, 0, 0, &iosb, BaseAddress, fsi.EndOfFile.LowPart, 0, 0)) ||
									0 > (status = ZwProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &RegionSize,
										PAGE_EXECUTE_READ, &fsi.NumberOfLinks)) ||
									0 > (status = RedirectContext(BaseAddress)))
								{
									ZwFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &RegionSize, MEM_RELEASE);
									BaseAddress = 0;
								}
							}
						}
					}

					ZwClose(hFile);
				}
			}
		}

		RtlNtStatusToDosError(status);
	}

	return 0 <= status;
}

void CALLBACK OnTls(PVOID, DWORD Reason, PVOID)
{
	switch (Reason)
	{
	case DLL_PROCESS_DETACH:
		FreeZwSupport();
		break;
	case DLL_PROCESS_ATTACH:
		OnProcessAttach();
		break;
	}
}

ULONG _tls_index;

static const PIMAGE_TLS_CALLBACK g_tls_cb[] = { OnTls, 0 };

#pragma const_seg(".rdata$T")

EXTERN_C const IMAGE_TLS_DIRECTORY _tls_used = {
	(ULONG_PTR)0,			// start of tls data
	(ULONG_PTR)0,			// end of tls data
	(ULONG_PTR)&_tls_index,	// address of tls_index
	(ULONG_PTR)g_tls_cb,	// pointer to call back array
};
#pragma const_seg()

#ifdef _WIN64
__pragma(comment(linker, "/include:_tls_used"))
#else 
__pragma(comment(linker, "/include:__tls_used"))
#endif

EXTERN_C
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlExitUserProcess(
	_In_ NTSTATUS ExitStatus
);

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseHardError(
	_In_ NTSTATUS ErrorStatus,
	_In_ ULONG NumberOfParameters,
	_In_ ULONG UnicodeStringParameterMask,
	_In_reads_(NumberOfParameters) PULONG_PTR Parameters,
	_In_ HARDERROR_RESPONSE_OPTION ValidResponseOptions,
	_Out_ HARDERROR_RESPONSE* Response
);

void WINAPI ep(HARDERROR_RESPONSE Response)
{
	NTSTATUS status = RtlGetLastNtStatus();
	if (0 <= status)
	{
		status = STATUS_UNSUCCESSFUL;
	}

	RtlExitUserProcess(NtRaiseHardError(status, 0, 0, 0, OptionOk, &Response));
}

_NT_END
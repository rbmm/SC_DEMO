#pragma once

struct IMP_HELP
{
	PVOID _M_hmod;
	PIMAGE_IMPORT_DESCRIPTOR _M_piid;

	void** _M_pFunction = 0;
	PIMAGE_THUNK_DATA _M_pThunk = 0;

	ULONG _M_n;

	PCSTR GetName(ULONG rva, PBOOL pbOrdinal);

	BOOL Init(PVOID hmod);

	NTSTATUS ProcessMAP(
		PCWSTR pszImp,
		PSTR pcsz,
		ULONG iSection,
		ULONG ofs,
		ULONG_PTR Va,
		ULONG s);

	NTSTATUS ProcessMAP(
		PCWSTR pszImp,
		PCWSTR pszMap,
		ULONG iSection,
		ULONG ofs,
		ULONG_PTR Va,
		ULONG s);

	PCSTR GetName(ULONG rva, DWORD FirstThunk, void** pFunction, PIMAGE_THUNK_DATA pThunk, PBOOL pbOrdinal);
};

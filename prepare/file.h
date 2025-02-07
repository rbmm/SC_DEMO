#pragma once

NTSTATUS ReadFromFile(_In_ PCWSTR lpFileName,
	_Out_ PBYTE* ppb,
	_Out_ ULONG* pcb,
	_In_opt_ ULONG cbBefore = 0,
	_In_opt_ ULONG cbAfter = 0);

NTSTATUS SaveToFile(
	_In_ PCWSTR lpFileName,
	_In_ const void* lpBuffer,
	_In_ ULONG nNumberOfBytesToWrite,
	_In_ BOOL MustBeEmpty = FALSE);
#include "stdafx.h"
#include "file.h"

NTSTATUS ReadFromFile(_In_ PCWSTR lpFileName,
	_Out_ PBYTE* ppb,
	_Out_ ULONG* pcb,
	_In_opt_ ULONG cbBefore /*= 0 */,
	_In_opt_ ULONG cbAfter /* = 0 */)
{
	UNICODE_STRING ObjectName;
	NTSTATUS status = RtlDosPathNameToNtPathName_U_WithStatus(lpFileName, &ObjectName, 0, 0);

	DbgPrint("DosPathNameToNt(\"%ws\") = %x\r\n", lpFileName, status);

	if (0 <= status)
	{
		HANDLE hFile;
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };
		IO_STATUS_BLOCK iosb;
		status = NtOpenFile(&hFile, FILE_GENERIC_READ, &oa, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

		DbgPrint("NtOpenFile(\"%wZ\") = %x\r\n", &ObjectName, status);

		RtlFreeUnicodeString(&ObjectName);

		if (0 <= status)
		{
			FILE_STANDARD_INFORMATION fsi;
			if (0 <= (status = NtQueryInformationFile(hFile, &iosb, &fsi, sizeof(fsi), FileStandardInformation)))
			{
				DbgPrint("FileSize=%I64u\r\n", fsi.EndOfFile.QuadPart);

				if (fsi.EndOfFile.QuadPart < 0x1000000)
				{
					if (PBYTE pb = new BYTE[cbBefore + fsi.EndOfFile.LowPart + cbAfter])
					{
						if (0 > (status = NtReadFile(hFile, 0, 0, 0, &iosb, pb + cbBefore, fsi.EndOfFile.LowPart, 0, 0)))
						{
							delete[] pb;
						}
						else
						{
							*ppb = pb;
							*pcb = (ULONG)iosb.Information;
						}

						DbgPrint("NtReadFile=%x\r\n", status);
					}
					else
					{
						status = STATUS_NO_MEMORY;
					}
				}
				else
				{
					status = STATUS_FILE_TOO_LARGE;
				}
			}

			NtClose(hFile);
		}
	}

	return status;
}

NTSTATUS SaveToFile(
	_In_ PCWSTR lpFileName,
	_In_ const void* lpBuffer,
	_In_ ULONG nNumberOfBytesToWrite,
	_In_ BOOL MustBeEmpty /*= FALSE*/)
{
	UNICODE_STRING ObjectName;
	NTSTATUS status = RtlDosPathNameToNtPathName_U_WithStatus(lpFileName, &ObjectName, 0, 0);

	DbgPrint("DosPathNameToNt(\"%ws\") = %x [%x]\r\n", lpFileName, status, MustBeEmpty);

	if (0 <= status)
	{
		HANDLE hFile;
		IO_STATUS_BLOCK iosb;
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

		LARGE_INTEGER AllocationSize = { nNumberOfBytesToWrite };

		if (MustBeEmpty)
		{
			if (0 <= (status = NtOpenFile(&hFile, FILE_READ_ATTRIBUTES, &oa, &iosb, FILE_SHARE_READ, 0)))
			{
				FILE_STANDARD_INFORMATION fsi;
				if (0 <= (status = NtQueryInformationFile(hFile, &iosb, &fsi, sizeof(fsi), FileStandardInformation)))
				{
					MustBeEmpty = fsi.EndOfFile.QuadPart != 0;
				}

				NtClose(hFile);
			}
		}

		status = NtCreateFile(&hFile, FILE_APPEND_DATA | SYNCHRONIZE, &oa, &iosb, &AllocationSize,
			0, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, 0, 0);

		DbgPrint("CreateFile(\"%wZ\") = %x [%x]\r\n", &ObjectName, status, MustBeEmpty);

		RtlFreeUnicodeString(&ObjectName);

		if (0 <= status)
		{
			status = MustBeEmpty ? STATUS_DIRECTORY_NOT_EMPTY :
				NtWriteFile(hFile, 0, 0, 0, &iosb, const_cast<void*>(lpBuffer), nNumberOfBytesToWrite, 0, 0);
			NtClose(hFile);
			DbgPrint("WriteFile(%x) = %x\r\n", nNumberOfBytesToWrite, status);
		}
	}

	return status;
}
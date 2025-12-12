#include "stdafx.h"
#include <conio.h>
#include "zip.h"

ULONG BOOL_TO_ERROR(BOOL f);

PSTR __fastcall strnchr(SIZE_T n1, const void* str1, char c);

volatile UCHAR guz;

bool gIsConsole = false;

HRESULT GetLastStatus(ULONG dwError = GetLastError())
{
	NTSTATUS status = RtlGetLastNtStatus();
	return RtlNtStatusToDosErrorNoTeb(status) == dwError ? HRESULT_FROM_NT(status) : HRESULT_FROM_WIN32(dwError);
}

void InitConsole(HWND hwnd)
{
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

	ULONG dwProcessList[2];
	if (GetConsoleProcessList(dwProcessList, RTL_NUMBER_OF(dwProcessList)) == 1)
	{
		NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };

		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
		{
			CONSOLE_FONT_INFOEX cf = {
				sizeof(cf), 0, { (SHORT)ncm.lfCaptionFont.lfWidth, -(SHORT)ncm.iMenuHeight},
				FF_MODERN, FW_NORMAL, L"Lucida Console"
			};

			SetCurrentConsoleFontEx(hStdout, FALSE, &cf);
			CONSOLE_SCREEN_BUFFER_INFOEX csbi = { sizeof(csbi) };
			if (GetConsoleScreenBufferInfoEx(hStdout, &csbi))
			{
				csbi.dwSize = csbi.dwMaximumWindowSize = GetLargestConsoleWindowSize(hStdout);
				csbi.wAttributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				csbi.dwCursorPosition.X = 0, csbi.dwCursorPosition.Y = 0;
				csbi.srWindow.Left = 0, csbi.srWindow.Top = 0;
				csbi.srWindow.Right = csbi.dwSize.X - 1, csbi.srWindow.Bottom = csbi.dwSize.Y - 1;
				SetConsoleScreenBufferInfoEx(hStdout, &csbi);
				SetConsoleWindowInfo(hStdout, TRUE, &csbi.srWindow);
			}
		}
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
	}
}

void InitConsole()
{
	if (HWND hwnd = GetConsoleWindow())
	{
		FILE_FS_DEVICE_INFORMATION ffdi;
		IO_STATUS_BLOCK iosb;

		if (gIsConsole = IsWindowVisible(hwnd) &&
			0 <= NtQueryVolumeInformationFile(GetStdHandle(STD_OUTPUT_HANDLE),
			&iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation) &&
			ffdi.DeviceType == FILE_DEVICE_CONSOLE &&
			0 <= NtQueryVolumeInformationFile(GetStdHandle(STD_INPUT_HANDLE),
			&iosb, &ffdi, sizeof(ffdi), FileFsDeviceInformation) &&
			ffdi.DeviceType == FILE_DEVICE_CONSOLE)
		{
			InitConsole(hwnd);
		}
	}
}

void cprintf(ULONG nStdHandle, PCWSTR pwz, ULONG len = 0)
{
	if (!len) len = (ULONG)wcslen(pwz);

	HANDLE hOut = GetStdHandle(nStdHandle);
	if (gIsConsole)
	{
		WriteConsoleW(hOut, pwz, len, &len, 0);
		return;
	}

	UINT cp = nStdHandle == STD_ERROR_HANDLE ? GetConsoleOutputCP() : CP_UTF8;

	if (ULONG cb = WideCharToMultiByte(cp, 0, pwz, len, 0, 0, 0, 0))
	{
		if (PSTR psz = (PSTR)_malloca(cb))
		{
			if (cb = WideCharToMultiByte(cp, 0, pwz, len, psz, cb, 0, 0))
			{
				WriteFile(hOut, psz, cb, &cb, 0);
			}

			_freea(psz);
		}
	}
}

void cprintf(PCWSTR pwz, ULONG len = 0)
{
	cprintf(STD_ERROR_HANDLE, pwz, len);
}

void vprintf(ULONG nStdHandle, PCWSTR format, va_list args)
{
	int len = _vscwprintf(format, args);

	if (0 < len)
	{
		if (PWSTR pwz = (PWSTR)_malloca((len + 1) * sizeof(WCHAR)))
		{
			if (0 < (len = _vsnwprintf_s(pwz, len + 1, _TRUNCATE, format, args)))
			{
				cprintf(nStdHandle, pwz, len);
			}

			_freea(pwz);
		}
	}
}

void printf(ULONG nStdHandle, PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(nStdHandle, format, args);
	va_end(args);
}

void printf(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(STD_ERROR_HANDLE, format, args);
	va_end(args);
}

void fprintf(PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(STD_OUTPUT_HANDLE, format, args);
	va_end(args);
}

void printf(HRESULT hr)
{
	printf(STD_ERROR_HANDLE, L"// %x\n", hr & ~FACILITY_NT_BIT);

	PWSTR psz;
	HMODULE hmod = 0;

	ULONG Flags = FORMAT_MESSAGE_IGNORE_INSERTS| FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	if (hr & FACILITY_NT_BIT)
	{
		Flags = FORMAT_MESSAGE_IGNORE_INSERTS| FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE;
		hr &= ~FACILITY_NT_BIT;
		hmod = GetModuleHandle(L"ntdll");
	}

	ULONG r = FormatMessage(Flags, hmod, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (PWSTR)&psz, 0, NULL);

	if (r)
	{
		cprintf(STD_ERROR_HANDLE, psz, r);
		LocalFree(psz);
	}
}

void printf_status(NTSTATUS status)
{
	printf(HRESULT_FROM_NT(status));
}

struct NtNameFromWin32 : public UNICODE_STRING
{
	NtNameFromWin32()
	{
		Buffer = 0, Length = 0, MaximumLength = 0;
	}

	~NtNameFromWin32()
	{
		RtlFreeUnicodeString(this);
	}

	NTSTATUS Set(PCWSTR DosFileName, PWSTR * FileName = 0)
	{
		RtlFreeUnicodeString(this);
		return RtlDosPathNameToNtPathName_U_WithStatus(DosFileName, this, FileName, 0);
	}
};

NTSTATUS CreateNewFile(PHANDLE FileHandle, POBJECT_ATTRIBUTES poa)
{
	IO_STATUS_BLOCK iosb;
	return NtCreateFile(FileHandle, FILE_APPEND_DATA|SYNCHRONIZE, poa, &iosb, 0,
		FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, 0, 0);
}

NTSTATUS CreateFileFromData(POBJECT_ATTRIBUTES poa, PVOID pv, ULONG cb)
{
	HANDLE hFile;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;
	if (0 <= (status = CreateNewFile(&hFile, poa)))
	{
		if (cb)
		{
			status = NtWriteFile(hFile, 0, 0, 0, &iosb, pv, cb, 0, 0);
		}
		NtClose(hFile);
	}

	return status;
}

NTSTATUS SaveF(const void* pv, ULONG cb, HANDLE hRoot, PCWSTR pszFileName)
{
	HANDLE hFile;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), hRoot, &ObjectName, OBJ_CASE_INSENSITIVE };

	LARGE_INTEGER AllocationSize = { cb };

	RtlInitUnicodeString(&ObjectName, pszFileName);

	NTSTATUS status = NtCreateFile(&hFile, FILE_APPEND_DATA | SYNCHRONIZE, &oa, &iosb, &AllocationSize,
		0, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, 0, 0);

	if (0 <= status)
	{
		status = cb ? NtWriteFile(hFile, 0, 0, 0, &iosb, const_cast<void*>(pv), cb, 0, 0) : STATUS_SUCCESS;
		NtClose(hFile);
	}

	return status;
}

NTSTATUS SaveToFile(_In_ PCWSTR lpFileName, _In_ const void* lpBuffer, _In_ ULONG nNumberOfBytesToWrite)
{
	UNICODE_STRING ObjectName;
	NTSTATUS status = RtlDosPathNameToNtPathName_U_WithStatus(lpFileName, &ObjectName, 0, 0);

	DbgPrint("DosPathNameToNt(\"%ws\") = %x\r\n", lpFileName, status);

	if (0 <= status)
	{
		HANDLE hFile;
		IO_STATUS_BLOCK iosb;
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

		LARGE_INTEGER AllocationSize = { nNumberOfBytesToWrite };

		status = NtCreateFile(&hFile, FILE_APPEND_DATA | SYNCHRONIZE, &oa, &iosb, &AllocationSize,
			0, 0, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, 0, 0);

		DbgPrint("CreateFile(\"%wZ\") = %x\r\n", &ObjectName, status);

		RtlFreeUnicodeString(&ObjectName);

		if (0 <= status)
		{
			status = NtWriteFile(hFile, 0, 0, 0, &iosb, const_cast<void*>(lpBuffer), nNumberOfBytesToWrite, 0, 0);
			NtClose(hFile);

			DbgPrint("WriteFile(%x) = %x\r\n", nNumberOfBytesToWrite, status);
		}
	}

	return status;
}

NTSTATUS CreateRoot(_Out_ PHANDLE FileHandle, _In_ PCWSTR lpFileName)
{
	UNICODE_STRING ObjectName;

	NTSTATUS status = RtlDosPathNameToNtPathName_U_WithStatus(lpFileName, &ObjectName, 0, 0);

	if (0 <= status)
	{
		IO_STATUS_BLOCK iosb;
		OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

		status = NtCreateFile(FileHandle, FILE_ADD_FILE | SYNCHRONIZE, &oa, &iosb, 0,
			FILE_ATTRIBUTE_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE, 0, 0);

		RtlFreeUnicodeString(&ObjectName);
	}

	return status;
}

HRESULT DropVcxProj(ULONG t,
					void* pv,
					ULONG cb,
					PCSTR pcszTargetName,
					HANDLE hRoot,
					PCWSTR pszFileName,
					void** ppv,
					ULONG* pcb)
{
	GUID guid;

	if (HRESULT hr = BCryptGenRandom(0, (PBYTE)&guid, sizeof(guid), BCRYPT_USE_SYSTEM_PREFERRED_RNG))
	{
		return hr;
	}

	char szGuid[39];
	int i = sprintf_s(szGuid, _countof(szGuid), "{%08X-%04x-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	if (0 < i)
	{
		PCSTR aa[] = { pcszTargetName, szGuid, pcszTargetName };
		ULONG l = (ULONG)strlen(pcszTargetName);
		ULONG len[] = { l, (ULONG)i, l };

		if (PSTR buf = new char[cb + 2*l + i])
		{
			PSTR psz = buf;

			while (PSTR pc = strnchr(cb, pv, '^'))
			{
				l = RtlPointerToOffset(pv, pc);

				cb -= l;

				memcpy(psz, pv, l - 1);

				pv = pc, psz += l - 1;

				if (!t--)
				{
					*ppv = pv, * pcb = cb;
					return SaveF(buf, RtlPointerToOffset(buf, psz), hRoot, pszFileName);
				}

				memcpy(psz, aa[t], l = len[t]);
				psz += l;
			}

			delete[] buf;
		}
	}

	return HRESULT_FROM_NT(STATUS_INTERNAL_ERROR);
}

HRESULT DropFoU(void* pv,
				ULONG cb,
				HANDLE hRoot,
				PCWSTR pszFileName,
				void** ppv,
				ULONG* pcb)
{
	if (PSTR pc = strnchr(cb, pv, '^'))
	{
		ULONG l = RtlPointerToOffset(pv, pc);
		*ppv = pc, * pcb = cb - l;

		return SaveF(pv, l - 1, hRoot, pszFileName);
	}

	return HRESULT_FROM_NT(STATUS_INTERNAL_ERROR);
}

extern const CHAR sc_vcxproj_begin[], sc_vcxproj_end[];

HRESULT NewVcxProj(PCWSTR pszProjectName)
{
	HANDLE hFolder;
	static const UNICODE_STRING CharSet = RTL_CONSTANT_STRING(L"\\/");

	UNICODE_STRING str;
	RtlInitUnicodeString(&str, pszProjectName);

	USHORT np;
	if (0 <= RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, &str, &CharSet, &np))
	{
		np += sizeof(WCHAR);
		(PBYTE&)str.Buffer += np;
		str.Length -= np, str.MaximumLength -= np;
	}

	PSTR Target = 0;
	ULONG cch = 0;
	ULONG len = str.Length / sizeof(WCHAR);

	while (cch = WideCharToMultiByte(CP_UTF8, 0, str.Buffer, len, Target, cch, 0, 0))
	{
		if (Target)
		{
			Target[cch] = 0;
			PWSTR buf = (PWSTR)alloca(len = str.Length + 32 * sizeof(WCHAR));

			len /= sizeof(WCHAR);

			int i = swprintf_s(buf, len, L"%wZ", &str);

			if (0 < i)
			{
				PWSTR psz = buf + i;

				HRESULT hr = CreateRoot(&hFolder, pszProjectName);

				printf(L"Create(\"%ws\")=%x\r\n", buf, hr);

				if (0 <= hr)
				{
					PVOID pv = const_cast<char*>(sc_vcxproj_begin);
					ULONG cb = RtlPointerToOffset(sc_vcxproj_begin, sc_vcxproj_end);

					if (S_OK == (hr = Unzip(pv, cb, &pv, &cb)))
					{
						PVOID Buf = pv;

						wcscpy(psz, L".vcxproj");
						psz += _countof(".vcxproj") - 1;

						if (S_OK == (hr = DropVcxProj(2, pv, cb, Target, hFolder, buf, &pv, &cb)))
						{
							*psz++ = '.';

							static const PCWSTR sc[] = { 
								L"imp.x86.asm", 
								L"imp.x64.asm", 
								L"x86.asm", 
								L"x64.asm", 
								L"ep.cpp", 
								L"stdafx.cpp", 
								L"stdafx.h", 
								L"user", 
								L"filters" 
							};

							i = _countof(sc);
							do
							{
								if (_countof(sc) - 2 == i)
								{
									psz = buf;
								}

								wcscpy(psz, sc[--i]);

								if (hr = DropFoU(pv, cb, hFolder, buf, &pv, &cb))
								{
									break;
								}
							} while (i);

						}

						LocalFree(Buf);
					}

					NtClose(hFolder);
				}

				return hr;
			}

			break;
		}

		Target = (PSTR)alloca(cch + 1);
	}

	return E_FAIL;
}

NTSTATUS SaveAsAsm(PCWSTR pwzFileName, const void* pcv, SIZE_T cb)
{
	NTSTATUS status;

	union {
		const void* pv;
		PULONG64 pu64;
		PULONG pu;
		PUSHORT ps;
		PUCHAR pb;
	};

	pv = pcv;

	SIZE_T n = cb >> 3;
	SIZE_T cch = n * (7 + 16) + 36;
	// DD 0ABCDEF78h.. ; 7+8
	// DW 0ABCDh..     ; 7+4
	// DB 0ABh..       ; 7+2

	if (PSTR buf = new char [cch])
	{
		status = STATUS_INTERNAL_ERROR;

		int len;
		ULONG s = 4;

		PSTR psz = buf;

		if (n)
		{
			cb -= n << 3;

			do
			{
				if (0 >= (len = sprintf_s(psz, cch, "DQ 0%016I64xh\r\n", *pu64++)))
				{
					goto __exit;
				}

			} while (psz += len, cch -= len, --n);
		}

		static const PCSTR fmt[] = { "DB 0%02xh\r\n", "DW 0%04xh\r\n", "DD 0%08xh\r\n" };
		n = _countof(fmt) - 1;

		do
		{
			if (s <= cb)
			{
				cb -= s;

				if (0 >= (len = sprintf_s(psz, cch, fmt[n], *pu & ((1ULL << (s << 3)) - 1))))
				{
					goto __exit;
				}

				psz += len, cch -= len, pb += s;
			}

		} while (s >>= 1, n--);

		status = SaveToFile(pwzFileName, buf, RtlPointerToOffset(buf, psz));

__exit:
		delete[] buf;

		DbgPrint("SaveAsAsm(%ws)=%x\r\n", pwzFileName, status);

		return status;
	}

	return STATUS_NO_MEMORY;
}

NTSTATUS ZipAndSaveAsAsm(PCWSTR pwzFileName, const void* pv, SIZE_T cb)
{
	COMPRESSOR_HANDLE CompressorHandle;
	if (CreateCompressor(COMPRESS_ALGORITHM_MSZIP, 0, &CompressorHandle))
	{
		ULONG dwError;
		SIZE_T CompressedDataSize;

		switch (dwError = BOOL_TO_ERROR(Compress(CompressorHandle, 0, cb, 0, 0, &CompressedDataSize)))
		{
		case NOERROR:
		case ERROR_INSUFFICIENT_BUFFER:
			if (PBYTE pb = new BYTE[CompressedDataSize])
			{
				if (Compress(CompressorHandle, pv, cb, pb, CompressedDataSize, &CompressedDataSize))
				{
					printf(L"Compress:%x >> %x [%u%%]\r\n", cb, CompressedDataSize, (CompressedDataSize*100)/cb);
					dwError = SaveAsAsm(pwzFileName, pb, CompressedDataSize);
				}
				else
				{
					dwError = GetLastError();
				}

				delete [] pb;
			}

			break;
		}

		CloseCompressor(CompressorHandle);

		return HRESULT_FROM_WIN32(dwError);
	}

	return HRESULT_FROM_WIN32(GetLastError());
}

#define FILE_SHARE_VALID_FLAGS 7

NTSTATUS ToZipAsm(PCWSTR from, PCWSTR to, bool _text, bool bZip)
{
	printf(L"ToAsm(\"%ws\", \"%ws\", %u)\n", from, to, _text);

	NtNameFromWin32 ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

	NTSTATUS status = ObjectName.Set(from);
	HANDLE hFile = 0, hSection = 0;
	IO_STATUS_BLOCK iosb;

	if (0 > status)
	{
		printf(L"convert <%s> = %x\n", from, status);
	}
	else
	{
		status = NtOpenFile(&hFile, FILE_GENERIC_READ, &oa, &iosb,
			FILE_SHARE_VALID_FLAGS, FILE_SYNCHRONOUS_IO_NONALERT);

		printf(L"OpenFile(%wZ) = %x\n", static_cast<PCUNICODE_STRING>(&ObjectName), status);
	}

	if (0 <= status)
	{
		FILE_STANDARD_INFORMATION fsi;

		if (0 <= (status = NtQueryInformationFile(hFile, &iosb, &fsi, sizeof(fsi), FileStandardInformation)))
		{
			printf(L"File Size = %I64x\n", fsi.EndOfFile.QuadPart);

			if (fsi.EndOfFile.LowPart - 1 < 0x1000000 && !fsi.EndOfFile.HighPart) // < 16mb
			{
				status = ZwCreateSection(&hSection, SECTION_ALL_ACCESS, 0, 0, PAGE_READONLY, SEC_COMMIT, hFile);
			}
			else
			{
				status = STATUS_EA_TOO_LARGE;
			}
		}

		NtClose(hFile);

		if (0 <= status)
		{
			void* BaseAddress=0;
			SIZE_T ViewSize = 0;

			status = ZwMapViewOfSection(hSection, NtCurrentProcess(), &BaseAddress, 
				0, 0, 0, &ViewSize, ViewUnmap, 0, PAGE_READONLY);

			NtClose(hSection);

			if (0 <= status)
			{
				union {
					PLONGLONG pl;
					PVOID pv;
					PBYTE pb;
				};

				pv = BaseAddress;

				if (_text)
				{
					status = STATUS_INVALID_IMAGE_FORMAT;

					__try 
					{
						if (PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(BaseAddress))
						{
							if (pinth->FileHeader.NumberOfSections)
							{
								PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);

								ULONG PointerToRawData = pish->PointerToRawData, SizeOfRawData = pish->SizeOfRawData;

								if (PointerToRawData < ViewSize &&
									PointerToRawData + SizeOfRawData <= ViewSize && 
									PointerToRawData < PointerToRawData + SizeOfRawData)
								{
									pb += PointerToRawData;
									fsi.EndOfFile.QuadPart = SizeOfRawData;
									status = STATUS_SUCCESS;
									printf(L".text(ofs=%x, size=%x)\n", PointerToRawData, SizeOfRawData);
								}
							}
						}
					} 
					__except(EXCEPTION_EXECUTE_HANDLER) 
					{
						status = GetExceptionCode();
					}
				}

				if (0 <= status)
				{
					status = (bZip ? ZipAndSaveAsAsm : SaveAsAsm)(to, pv, fsi.EndOfFile.LowPart);
				}

				ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
			}
		}
	}

	return status;
}

NTSTATUS SetPETime(PCWSTR lpFileName, PCWSTR time)
{
	printf(L"SetPETime(%ws, %ws)\r\n", lpFileName, time);
	ULONG TimeDateStamp = wcstoul(time, const_cast<WCHAR**>(&time), 16);
	if (!TimeDateStamp || *time)
	{
		return STATUS_INVALID_PARAMETER_2;
	}
	NtNameFromWin32 ObjectName;
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, &ObjectName, OBJ_CASE_INSENSITIVE };

	NTSTATUS status = ObjectName.Set(lpFileName);
	HANDLE hFile = 0;
	IO_STATUS_BLOCK iosb;

	if (0 > status)
	{
		printf(L"convert <%s> = %x\r\n", lpFileName, status);
	}
	else
	{
		status = NtOpenFile(&hFile, FILE_GENERIC_READ | FILE_WRITE_DATA, &oa, &iosb,
			FILE_SHARE_VALID_FLAGS, FILE_SYNCHRONOUS_IO_NONALERT);

		printf(L"OpenFile(%wZ) = %x\r\n", static_cast<PCUNICODE_STRING>(&ObjectName), status);
	}

	if (0 <= status)
	{
		union {
			IMAGE_DOS_HEADER idh;
			IMAGE_NT_HEADERS inh;
		};

		if (0 <= (status = NtReadFile(hFile, 0, 0, 0, &iosb, &idh, sizeof(idh), 0, 0)))
		{
			if (IMAGE_DOS_SIGNATURE == idh.e_magic)
			{
				LARGE_INTEGER bo = { (ULONG)idh.e_lfanew };
				if (0 <= (status = NtReadFile(hFile, 0, 0, 0, &iosb, &inh, sizeof(inh), &bo, 0)))
				{
					if (IMAGE_NT_SIGNATURE == inh.Signature)
					{
						ULONG t = inh.FileHeader.TimeDateStamp;
						inh.FileHeader.TimeDateStamp = TimeDateStamp;
						status = NtWriteFile(hFile, 0, 0, 0, &iosb, &inh, sizeof(inh), &bo, 0);
						printf(L"TimeDateStamp: %08X <- %08X =[%x]\r\n", t, TimeDateStamp, status);
					}
					else
					{
						status = STATUS_INVALID_IMAGE_FORMAT;
					}
				}
			}
			else
			{
				status = STATUS_INVALID_IMAGE_NOT_MZ;
			}
		}

		NtClose(hFile);
	}

	return status;
}

void WINAPI ep(void*)
{
	InitConsole();

	HRESULT exitcode = HRESULT_FROM_NT(STATUS_INVALID_PARAMETER);

	BOOL bWait = TRUE;

	PVOID ov;
	Wow64DisableWow64FsRedirection(&ov);
	PVOID stack = alloca(guz);

	PWSTR* argv = (PWSTR*)stack, lpsz = GetCommandLineW(), c = wcsrchr(lpsz, '\"');
	if (c && !c[1]) *c=0;

	cprintf(lpsz);
	cprintf(L"\n");

	int argc = 0;

	while (lpsz = wcschr(lpsz, L'*'))
	{
		*lpsz++ = 0;

		if (--argv < stack) stack = alloca(sizeof(PVOID));

		*argv = lpsz, argc++;
	}

	while (argc--)
	{
		lpsz = *argv++;

		if (!wcscmp(lpsz, L"nowait"))
		{
			bWait = FALSE;
			continue;
		}

		exitcode = HRESULT_FROM_NT(STATUS_INVALID_PARAMETER);

		if (!wcscmp(lpsz, L"toasm"))
		{
			if (argc >= 2)
			{
				bool _text = (argc -= 2) && !wcscmp(argv[2], L".text");

				exitcode = HRESULT_FROM_NT(ToZipAsm(argv[0], argv[1], _text, FALSE)), argv += 2 + _text, argc -= _text;
			}
		}
		else if (!wcscmp(lpsz, L"tozasm"))
		{
			if (argc >= 2)
			{
				bool _text = (argc -= 2) && !wcscmp(argv[2], L".text");

				exitcode = HRESULT_FROM_NT(ToZipAsm(argv[0], argv[1], _text, TRUE)), argv += 2 + _text, argc -= _text;
			}
		}
		else if (!wcscmp(lpsz, L"vcp"))
		{
			if (argc)
			{
				exitcode = HRESULT_FROM_NT(NewVcxProj(argv[0]));
				argc--, argv++;
			}
		}
		else if (!wcscmp(lpsz, L"ts"))
		{
			if (1 < argc)
			{
				exitcode = SetPETime(argv[0], argv[1]);
				argc -= 2, argv += 2;
			}
		}

		if (exitcode & ~FACILITY_NT_BIT) break;
	}

	printf(exitcode);

	if (bWait)
	{
		if (gIsConsole)
		{
			cprintf(L"press any key...\n");
			_getch();
		}
	}

	ExitProcess(exitcode & ~FACILITY_NT_BIT);
}

#include "stdafx.h"
#include "print.h"

void PrintUTF8(PCSTR pcsz, ULONG len)
{
	HANDLE hFile = GetStdHandle(STD_OUTPUT_HANDLE);

	switch ((ULONG_PTR)hFile)
	{
	case 0:
	case (ULONG_PTR)INVALID_HANDLE_VALUE:
		return;
	}

	WriteFile(hFile, pcsz, len, &len, 0);
}

void PrintUTF8_v(PCSTR format, ...)
{
	va_list ap;
	va_start(ap, format);

	PSTR buf = 0;
	int len = 0;
	while (0 < (len = _vsnprintf(buf, len, format, ap)))
	{
		if (buf)
		{
			PrintUTF8(buf, len);
			break;
		}

		buf = (PSTR)alloca(++len);
	}
}

HRESULT PrintError(HRESULT dwError)
{
	LPCVOID lpSource = 0;
	ULONG dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

	if ((dwError & FACILITY_NT_BIT) || (0 > dwError && HRESULT_FACILITY(dwError) == FACILITY_NULL))
	{
		dwError &= ~FACILITY_NT_BIT;
	__nt:
		dwFlags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;

		lpSource = GetModuleHandle(L"ntdll");
	}

	PSTR lpText;
	if (ULONG cch = FormatMessageA(dwFlags, lpSource, dwError, 0, (PSTR)&lpText, 0, 0))
	{
		PrintUTF8(lpText, cch);
		LocalFree(lpText);
	}
	else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
	{
		goto __nt;
	}

	DbgPrint("0x%x (%d)\r\n", dwError, dwError);
	return dwError;
}
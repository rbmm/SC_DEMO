#pragma once

void PrintUTF8(PCSTR pcsz, ULONG len);

inline void PrintUTF8(PCSTR pcsz)
{
	PrintUTF8(pcsz, (ULONG)strlen(pcsz));
}

void PrintUTF8_v(PCSTR format, ...);

HRESULT PrintError(HRESULT dwError);

#define DbgPrint PrintUTF8_v
#include "stdafx.h"
#include "undname.h"

#ifdef _X86_

#include "rtlframe.h"

typedef RTL_FRAME<DATA_BLOB> AFRAME;

static void* __cdecl fAlloc(ULONG cb)
{
	if (DATA_BLOB* prf = AFRAME::get())
	{
		if (cb > prf->cbData)
		{
			return 0;
		}
		prf->cbData -= cb;
		PVOID pv = prf->pbData;
		prf->pbData += cb;
		return pv;
	}

	return 0;
}

static void __cdecl fFree(void*)
{
}

EXTERN_C
_CRTIMP
PSTR __cdecl __unDNameEx
(
	PSTR buffer,
	PCSTR mangled,
	DWORD cb,
	void* (__cdecl* memget)(DWORD),
	void(__cdecl* memfree)(void*),
	PSTR(__cdecl* GetParameter)(long i),
	DWORD flags
);

EXTERN_C PVOID _imp____unDNameEx = 0;

PSTR __cdecl GetParameter(long /*i*/)
{
	return const_cast<PSTR>("");
}

static PSTR _unDName(PCSTR mangled, PSTR buffer, DWORD cb, DWORD flags)
{
	if (_imp____unDNameEx)
	{
	__ok:
		AFRAME af;
		af.cbData = 32 * PAGE_SIZE;
		af.pbData = (PUCHAR)alloca(32 * PAGE_SIZE);

		return __unDNameEx(buffer, mangled, cb, fAlloc, fFree, GetParameter, flags);
	}

	if (HMODULE hmod = LoadLibraryW(L"msvcrt.dll"))
	{
		if (_imp____unDNameEx = GetProcAddress(hmod, "__unDNameEx"))
		{
			goto __ok;
		}
	}

	return 0;
}

PCSTR unDNameEx(_In_ PCSTR DecoratedName, _Out_ PSTR outputString, _In_ DWORD maxStringLength, _In_ DWORD flags)
{
	if ('?' != *DecoratedName)
	{
		return DecoratedName;
	}
	PSTR sz = _unDName(DecoratedName, outputString, maxStringLength, flags);
	return sz ? sz : DecoratedName;
}

#define CASE_XY(x, y) case x: c = y; break

PSTR UndecorateString(_In_ PSTR pszSym, _Out_opt_ PCSTR* ppszSection /*= 0*/)
{
	BOOL bUnicode;
	PSTR pc = pszSym, name = pszSym;

	switch (*pc++)
	{
	case '0':
		bUnicode = FALSE;
		break;

	case '1':
		bUnicode = TRUE;
		break;

	default:
		//__debugbreak();
		return 0;
	}

	if (*pc - '0' >= 10 && !(pc = strchr(pc, '@')))
	{
		//__debugbreak();
		return 0;
	}

	if (pc = strchr(pc + 1, '@'))
	{
		if (bUnicode)
		{
			*pszSym++ = 'L';
		}
		*pszSym++ = '\"';
	}
	else
	{
		//__debugbreak();
		return 0;
	}

	int i = 0;
	char c;

	while ('@' != (c = *++pc))
	{
		// special char ?
		union {
			USHORT u = 0;
			char pp[2];
		};

		if ('?' == c)
		{
			switch (*++pc)
			{
			case '$':
				pp[1] = *++pc, pp[0] = *++pc;

				switch (u)
				{
					CASE_XY('AA', 0);
					CASE_XY('AH', '.');//\a
					CASE_XY('AI', '.');//\b
					CASE_XY('AM', '.');//\f
					CASE_XY('AL', '.');//\v
					CASE_XY('AN', '.');//\r
					CASE_XY('CC', '\"');
					CASE_XY('HL', '{');
					CASE_XY('HN', '}');
					CASE_XY('FL', '[');
					CASE_XY('FN', ']');
					CASE_XY('CI', '(');
					CASE_XY('CJ', ')');
					CASE_XY('DM', '<');
					CASE_XY('DO', '>');
					CASE_XY('GA', '`');
					CASE_XY('CB', '!');
					CASE_XY('EA', '@');
					CASE_XY('CD', '#');
					CASE_XY('CF', '%');
					CASE_XY('FO', '^');
					CASE_XY('CG', '&');
					CASE_XY('CK', '*');
					CASE_XY('CL', '+');
					CASE_XY('HO', '~');
					CASE_XY('DN', '=');
					CASE_XY('HM', '|');
					CASE_XY('DL', ';');
					CASE_XY('DP', '?');
				default:
					return 0;
				}
				break;
				CASE_XY('0', ',');
				CASE_XY('1', '/');
				CASE_XY('2', '\\');
				CASE_XY('3', ':');
				CASE_XY('4', '.');
				CASE_XY('5', ' ');
				CASE_XY('6', '.');//\n
				CASE_XY('7', '.');//\t
				CASE_XY('8', '\'');
				CASE_XY('9', '-');
			case '@':
				//__debugbreak();
			default:
				return 0;
			}
		}

		if (bUnicode)
		{
			if (++i & 1)
			{
				if (c)
				{
					//__debugbreak();
					return 0;
				}
				continue;
			}
		}

		*pszSym++ = c;
	}

	*pszSym++ = '\"', *pszSym = 0;

	if (ppszSection)
	{
		*ppszSection = 0;
	}

	if (*++pc)
	{
		if (PSTR pa = strchr(pc, '@'))
		{
			*pa++ = 0;

			if (*pa)
			{
				//__debugbreak();
				return 0;
			}

			if (ppszSection)
			{
				*ppszSection = pc;
			}
		}
		else
		{
			//__debugbreak();
			return 0;
		}
	}

	return name;
}

#endif
#include "stdafx.h"

#ifdef _X86_

#include "map2.h"
#include "file.h"

struct MAP::RO
{
	ULONG rva, ofs;

	static int __cdecl compare(const RO* pa, const RO* pb)
	{
		if (pa->rva < pb->rva) return -1;
		if (pa->rva > pb->rva) return +1;
		return 0;
	}
};

MAP::RO* MAP::Parse(PSTR pcsz, PIMAGE_NT_HEADERS pinth)
{
	PSTR buf = pcsz;

	ULONG n = 0;
	ULONG64 u;
	ULONG_PTR ImageBase = 0;
__0:

	static const char plai[] = " Preferred load address is ";

	if (!memcmp(pcsz, plai, _countof(plai) - 1))
	{
		if (ImageBase)
		{
			return 0;
		}

		u = _strtoui64(pcsz + _countof(plai) - 1, const_cast<char**>(&pcsz), 16);

		if (!u || '\r' != *pcsz++ || '\n' != *pcsz++)
		{
			return 0;
		}

		ImageBase = (ULONG_PTR)u;

		goto __0;
	}

	ULONG s = strtoul(pcsz, const_cast<char**>(&pcsz), 16);

	if (':' != *pcsz)
	{
	__1:
		if (!(pcsz = strchr(pcsz, '\r')) || '\n' != *++pcsz)
		{
			return 0;
		}

		if (!*++pcsz)
		{
			if (n)
			{
				_M_n = n;
				return (RO*)(((ULONG_PTR)buf + (__alignof(RO) - 1)) & ~(__alignof(RO) - 1));
			}

			return 0;
		}
		goto __0;
	}

	ULONG o = strtoul(pcsz + 1, const_cast<char**>(&pcsz), 16);

	if (' ' != *pcsz)
	{
		return 0;
	}

	if ('0' == *++pcsz)
	{
		ULONG i = strtoul(pcsz + 1, &pcsz, 16);

		// <iSection>:<ofs> <s>H segment class
		if ('H' != *pcsz)
		{
			return 0;
		}

		//skip space
		while (' ' == *++pcsz);

		PCSTR pszSegmentName = pcsz;

		while (' ' != *++pcsz);

		*pcsz++ = 0;
		if (!strcmp(pszSegmentName, ".text$mn$cpp$u"))
		{
			if (--s < pinth->FileHeader.NumberOfSections)
			{
				PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth) + s;
				if (o < pish->Misc.VirtualSize && (o + i) <= pish->Misc.VirtualSize)
				{
					_M_rvaU = pish->VirtualAddress + o;
					_M_sizeU = i;
				}
			}
		}

		goto __1;
	}

	//skip space
	while (' ' == *++pcsz);

	PSTR pszFunc = pcsz;

	while (' ' != *++pcsz);

	*pcsz = 0;

	u = _strtoui64(pcsz + 1, &pcsz, 16);

	if (' ' != *pcsz || !ImageBase)
	{
		return 0;
	}

	if (u <= ImageBase)
	{
		goto __1;
	}

	if (0x1000000 < (u -= ImageBase))
	{
		goto __1;
	}

	if (!memcmp("??_C@_", pszFunc, 6))
	{
		pszFunc += 6;
		PSTR pa, pc;
		switch (*pszFunc++)
		{
		case '1':
			if (pc = strchr(pszFunc, '?'))
			{
				pszFunc = pc - 2;
				*pszFunc = 'L';
				pszFunc[1] = '\"';
				if (pa = strrchr(pc, '@'))
				{
					*pa = 0;
					pa = pc;
					// ?$AA
					while (pc && '?' == *pc++ && '$' == *pc++ && 'A' == *pc++ && 'A' == *pc++)
					{
						char c = *pc++;
						if ('?' == c)
						{
							pc = strchr(pc, '?');
							c = '.';
						}
						*pa++ = c;
					}
					*pa++ = '\"', *pa = 0;
				}
			}
			break;
		case '0':
			if (pc = strchr(pszFunc, '?'))
			{
				// O@HELLMLPH@ApcTest?
				*pc = '\"', * ++pc = 0;
				if (pc = strrchr(pszFunc, '@'))
				{
					*pc = '\"';
					pszFunc = pc;
				}
			}
			// pszFunc = L@HOFGLFDL@LdrLoadDll@
			else if (pc = strchr(pszFunc, '@'))
			{
				// pc = @HOFGLFDL@LdrLoadDll@
				if (pc = strchr(pc + 1, '@'))
				{
					// pc = @LdrLoadDll@
					*pc = '\"';
					pszFunc = pc;

					if (pc = strchr(pc + 1, '@'))
					{
						// pc = @
						*pc = '\"', * ++pc = 0;
					}
				}
			}
			break;
		}
	}

	int len = sprintf_s(buf, pszFunc - buf, "%x%c%hs%c", (ULONG)u, 0, pszFunc, 0);

	if (0 >= len)
	{
		return 0;
	}

	buf += len, n++;
	goto __1;
}

BOOL MAP::Init(PCWSTR pszMap, PIMAGE_NT_HEADERS pinth)
{
	PBYTE pb;
	ULONG cb;

	if (0 <= ReadFromFile(pszMap, &pb, &cb, 0, 1))
	{
		pb[cb] = 0;

		if (RO* pr = Parse((PSTR)pb, pinth))
		{
			ULONG n = _M_n;

			if (sizeof(RO) * n <= (cb - RtlPointerToOffset(pb, pr)))
			{
				_M_pr = pr;

				PSTR pc = (PSTR)pb;
				do
				{
					ULONG u = strtoul(pc, &pc, 16);
					if (*pc)
					{
						break;
					}
					pr->rva = u;
					pr++->ofs = RtlPointerToOffset(pb, ++pc);
					pc += strlen(pc) + 1;
				} while (--n);

				if (!n)
				{
					qsort(pr = _M_pr, n = _M_n, sizeof(RO), (int(__cdecl*)(const void*, const void*))RO::compare);
					
					_M_minRVA = pr->rva, _M_maxRVA = pr[n - 1].rva;

					_M_buf = (PSTR)pb;

					return TRUE;
				}
			}
		}

		delete[] pb;
	}

	return FALSE;
}

PCSTR MAP::GetName(_In_ ULONG rva, _Out_ ULONG* d)
{
	ULONG a = 0, o, b = _M_n;
	RO* pr = _M_pr;

	if (rva < _M_minRVA || _M_maxRVA <= rva)
	{
		*d = rva;
		return "";
	}

	do
	{
		ULONG r = pr[o = (a + b) >> 1].rva;
		if (rva == r)
		{
			*d = 0;
			return _M_buf + pr[o].ofs;
		}
		rva < r ? b = o : a = o + 1;
	} while (a < b);

	*d = rva - pr[--a].rva;

	return _M_buf + pr[a].ofs;
}

#endif // _X86_
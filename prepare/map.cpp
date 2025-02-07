#include "stdafx.h"
#include "map.h"
#include "file.h"

BOOL IMP_HELP::Init(PVOID hmod)
{
	ULONG s;
	if (PIMAGE_IMPORT_DESCRIPTOR piid = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
		hmod, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &s))
	{
		if (s /= sizeof(IMAGE_IMPORT_DESCRIPTOR))
		{
			_M_n = s, _M_piid = piid, _M_hmod = hmod;

			return TRUE;
		}
	}

	return FALSE;
}

PCSTR IMP_HELP::GetName(ULONG rva, DWORD FirstThunk, void** pFunction, PIMAGE_THUNK_DATA pThunk, PBOOL pbOrdinal)
{
	while (void* Function = *pFunction++)
	{
		IMAGE_THUNK_DATA Thunk = *pThunk++;

		if (rva == FirstThunk)
		{
			_M_pFunction = pFunction;
			_M_pThunk = pThunk;

			if (IMAGE_SNAP_BY_ORDINAL(Thunk.u1.Ordinal))
			{
				*pbOrdinal = TRUE;
				return (PCSTR)IMAGE_ORDINAL(Thunk.u1.Ordinal);
			}

			*pbOrdinal = FALSE;
			return (PCSTR)reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
				RtlOffsetToPointer(_M_hmod, Thunk.u1.AddressOfData))->Name;
		}

		FirstThunk += sizeof(PVOID);
	}

	_M_pFunction = 0;
	_M_pThunk = 0;
	return 0;
}

PCSTR IMP_HELP::GetName(ULONG rva, PBOOL pbOrdinal)
{
	PVOID hmod = _M_hmod;
	ULONG n = _M_n;
	PIMAGE_IMPORT_DESCRIPTOR piid = _M_piid;

	if (void** pFunction = _M_pFunction)
	{
		if (PCSTR name = GetName(rva, RtlPointerToOffset(hmod, pFunction), pFunction, _M_pThunk, pbOrdinal))
		{
			return name;
		}
	}

	do
	{
		DWORD Name = piid->Name;

		if (!Name)
		{
			return 0;
		}

		if (DWORD FirstThunk = piid->FirstThunk)
		{
			if (rva < FirstThunk)
			{
				continue;
			}

			if (DWORD OriginalFirstThunk = piid->OriginalFirstThunk)
			{
				if (PCSTR name = GetName(rva, FirstThunk,
					(void**)RtlOffsetToPointer(hmod, FirstThunk),
					(PIMAGE_THUNK_DATA)RtlOffsetToPointer(hmod, OriginalFirstThunk), pbOrdinal))
				{
					return name;
				}
			}
		}

	} while (piid++, --n);

	return 0;
}

ULONG HashString(PCSTR lpsz, ULONG hash = 0)
{
	while (char c = *lpsz++) hash = hash * 33 ^ c;
	return hash;
}

ULONG HashString(PCWSTR lpsz, ULONG hash = 0)
{
	while (WCHAR c = *lpsz++) hash = hash * 33 ^ c;
	return hash;
}

NTSTATUS IMP_HELP::ProcessMAP(
	PCWSTR pszImp,
	PSTR pcsz,
	ULONG iSection,
	ULONG ofs,
	ULONG_PTR Va,
	ULONG s)
{
	PVOID hmod = _M_hmod;
	PSTR buf = pcsz, psz = buf;

	ULONG64 u;
	BOOL f = FALSE;
	ULONG_PTR ImageBase = 0;
__0:

	PSTR pcszLine = pcsz;

	static const char plai[] = " Preferred load address is ";

	if (!f && !memcmp(pcsz, plai, _countof(plai) - 1))
	{
		if (ImageBase)
		{
			return STATUS_BAD_DATA;
		}

		u = _strtoui64(pcsz + _countof(plai) - 1, const_cast<char**>(&pcsz), 16);

		if (!u || '\r' != *pcsz++ || '\n' != *pcsz++)
		{
			return STATUS_BAD_DATA;
		}

		ImageBase = (ULONG_PTR)u;
		Va += ImageBase;

		DbgPrint("ImageBase = %p, Va = %p\r\n", ImageBase, Va);

		goto __0;
	}

	ULONG i = strtoul(pcsz, const_cast<char**>(&pcsz), 16);

	if (':' != *pcsz)
	{
	__1:
		if (!(pcsz = strchr(pcsz, '\r')) || '\n' != *++pcsz)
		{
			return STATUS_BAD_DATA;
		}

		++pcsz;
		goto __0;
	}

	if (i != iSection)
	{
		goto __1;
	}

	i = strtoul(pcsz + 1, const_cast<char**>(&pcsz), 16);

	if (' ' != *pcsz)
	{
		return STATUS_BAD_DATA;
	}

	if (i != ofs)
	{
		goto __1;
	}

	if (f)
	{
		pcsz = pcszLine;
	}
	else
	{
		// <iSection>:<ofs> <s>H .idata$5                DATA
		i = strtoul(pcsz + 1, &pcsz, 16);

		if ('H' != *pcsz || s != i)
		{
			return STATUS_BAD_DATA;
		}

		f = TRUE;
		goto __1;
	}

	//////////////////////////////////////////////////////////////////////////

	NTSTATUS status;

__loop:
	void** ppfn = (void**)RtlOffsetToPointer(hmod, Va - ImageBase);
	void* pfn = *ppfn;

	i = strtoul(pcsz, const_cast<char**>(&pcsz), 16);

	if (':' != *pcsz || iSection != i)
	{
		return STATUS_BAD_DATA;
	}

	i = strtoul(pcsz + 1, const_cast<char**>(&pcsz), 16);

	if (' ' != *pcsz || ofs != i)
	{
		//while (!IsDebuggerPresent()) Sleep(1000); __debugbreak();
		// zero end IAT
		do
		{
			if (*ppfn++)
			{
				DbgPrint("##!! ofs(%p) != i(%p) !!##\r\n", ofs, i);
				return STATUS_BAD_DATA;
			}

		} while (s -= sizeof(PVOID));

	__ok:
		status = SaveToFile(pszImp, buf, RtlPointerToOffset(buf, psz), TRUE);
		return 0 > status ? status : STATUS_MORE_PROCESSING_REQUIRED;
	}

	ofs += sizeof(PVOID);
	BOOL __imp_ = FALSE;

__space:
	switch (*pcsz++)
	{
	case ' ':
		goto __space;

	default:
		return STATUS_BAD_DATA;

	case '_':
		if (memcmp(pcsz, "_imp_", 5))
		{
			return STATUS_BAD_DATA;
		}

		pcsz += 5;
		__imp_ = TRUE;

	case '\\':
		INT k = -1;

		PCSTR name = pcsz;

		while (' ' < *pcsz) pcsz++;

		*pcsz++ = 0;

		if (!strcmp(name, "__chkstk") || !strcmp(name, "__alloca_probe"))
		{
			DbgPrint("!!!! %hs\\%hs imported !!!!!\r\n", "__chkstk", "__alloca_probe");
			return STATUS_BAD_DATA;
		}

		u = _strtoui64(pcsz, const_cast<char**>(&pcsz), 16);

		if (Va != u)
		{
			DbgPrint("##!! Va(%p) != u(%p) !!##\r\n", Va, u);
			return STATUS_BAD_DATA;
		}

		Va += sizeof(PVOID);

		PCSTR pszLib = 0;

	__00:
		switch (*pcsz)
		{
		case 0:
			return STATUS_BAD_DATA;
		case ':':
			*pcsz++ = 0;
			goto __01;
		default:
			if (!pszLib)
			{
				pszLib = pcsz;
			}
		case ' ':
			pcsz++;
			goto __00;
		}
	__01:

		PCSTR pszDLL = pcsz;

	__02:
		switch (*pcsz++)
		{
		case 0:
		case ':':
			return STATUS_BAD_DATA;
		default:
			goto __02;
		case '\r':
			pcsz[-1] = 0;
			break;
		}

		if ('\n' != *pcsz++)
		{
			return STATUS_BAD_DATA;
		}

		ULONG cb = RtlPointerToOffset(psz, name);

		if (__imp_)
		{
			if (!pfn)
			{
				DbgPrint("##!! pfn !!## at %p\r\n", u);
				return STATUS_BAD_DATA;
			}

			__imp_ = FALSE;

			BOOL bOrdinal;
			if (PCSTR str = GetName((ULONG)(u - ImageBase), &bOrdinal))
			{
				k = sprintf_s(psz, cb, "createFunc %hs, %hs, 0%08xh\r\n", pszLib, name,
					bOrdinal ? 0x80000000 | (ULONG)(ULONG_PTR)str : HashString(str));
			}
			else
			{
				return STATUS_BAD_DATA;
			}
		}
		else
		{
			if (pfn)
			{
				DbgPrint("##!! pfn=%p !!## at %p\r\n", pfn, u);
				return STATUS_BAD_DATA;
			}

			if (!_stricmp(pszDLL, "ntdll.dll"))
			{
				pszDLL = "";
			}

			k = sprintf_s(psz, cb, "\r\nHMOD %hs, <%hs>\r\n\r\n", pszLib, pszDLL);
		}

		if (0 >= k)
		{
			return STATUS_INTERNAL_ERROR;
		}

		psz += k;

		if (s -= sizeof(PVOID))
		{
			goto __loop;
		}

		goto __ok;
	}
}

NTSTATUS IMP_HELP::ProcessMAP(
	PCWSTR pszImp,
	PCWSTR pszMap,
	ULONG iSection,
	ULONG ofs,
	ULONG_PTR Va,
	ULONG s)
{
	PBYTE pb;
	ULONG cb;

	DbgPrint("ProcessMAP(%04x:%08x %p [%x])...\r\n", iSection, ofs, Va, s);

	NTSTATUS status = ReadFromFile(pszMap, &pb, &cb, 0, 1);

	if (0 <= status)
	{
		pb[cb - 1] = 0;
		status = ProcessMAP(pszImp, (PSTR)pb, iSection, ofs, Va, s);
		delete[] pb;
	}

	return status;
}

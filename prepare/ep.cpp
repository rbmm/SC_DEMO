#include "stdafx.h"

#include "print.h"
#include "file.h"
#include "map.h"
#include "map2.h"
#include "undname.h"

NTSTATUS ProcessIAT(PVOID hmod, PWSTR pczObj, PCWSTR pszImp, PCWSTR pszMap, ULONG_PTR pvShellEnd)
{
	ULONG s;

	union {
		PVOID pv;
		PBYTE pb;
		PIMAGE_BASE_RELOCATION pibr;
	};

	if (pv = RtlImageDirectoryEntryToData(hmod, TRUE, IMAGE_DIRECTORY_ENTRY_IAT, &s))
	{
		DbgPrint("IAT: %p [%x]\r\n", pv, s);

		if (!s || (s & (sizeof(PVOID) - 1)))
		{
			return STATUS_INTERNAL_ERROR;
		}

		DbgPrint("Delete(\"%ws\")=%x\r\n", pczObj, DeleteFileW(pczObj) ? 0 : RtlGetLastNtStatus());

		if (PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(hmod))
		{
			if (ULONG NumberOfSections = pinth->FileHeader.NumberOfSections)
			{
				ULONG_PTR Rva = (ULONG_PTR)pv - (ULONG_PTR)hmod;

				ULONG iSection = 0;
				PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinth);
				do
				{
					++iSection;

					ULONG_PTR Ofs = Rva - pish->VirtualAddress;

					if (Ofs < pish->Misc.VirtualSize)
					{
						if (Ofs + s <= pish->Misc.VirtualSize)
						{
							IMP_HELP imh;
							if (imh.Init(hmod))
							{
								return imh.ProcessMAP(pszImp, pszMap, iSection, (ULONG)Ofs, Rva, s);
							}
						}

						break;
					}
				} while (pish++, --NumberOfSections);
			}
		}

		return STATUS_INTERNAL_ERROR;
	}

	if ((pv = RtlImageDirectoryEntryToData(hmod, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &s)) && s)
	{
		pvShellEnd -= (ULONG_PTR)hmod;

		//while (!IsDebuggerPresent()) Sleep(1000); __debugbreak();
		MAP map;
		map.Init(pszMap, RtlImageNtHeader(hmod));

		do
		{
			if (s < sizeof(IMAGE_BASE_RELOCATION))
			{
				return STATUS_INVALID_IMAGE_FORMAT;
			}

			ULONG SizeOfBlock = pibr->SizeOfBlock;

			if (SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) || s < SizeOfBlock)
			{
				return STATUS_INVALID_IMAGE_FORMAT;
			}

			ULONG VirtualAddress = pibr->VirtualAddress;

			struct TYPE_OFFSET
			{
				WORD ofs : 12;
				WORD type : 4;
			}*pu = (TYPE_OFFSET*)(pibr + 1);

			pb += SizeOfBlock, s -= SizeOfBlock, SizeOfBlock -= sizeof(IMAGE_BASE_RELOCATION);

			if (SizeOfBlock & (sizeof(WORD) - 1))
			{
				return STATUS_INVALID_IMAGE_FORMAT;
			}

			SizeOfBlock >>= 1;

			do
			{
				if (pu->type)
				{
					//DbgPrint("\t## %x %08x\r\n", pu->type, VirtualAddress + pu->ofs);

					if (VirtualAddress + pu->ofs < pvShellEnd)
					{
						ULONG rva = VirtualAddress + pu->ofs;
						union {
							PVOID prv;
							PBYTE prb;
							PULONG_PTR pru;
						};
						prv = RtlOffsetToPointer(hmod, rva);

#ifdef _X86_
#define IMAGE_REL_BASED IMAGE_REL_BASED_HIGHLOW
#else
#define IMAGE_REL_BASED IMAGE_REL_BASED_DIR64
#endif // _X86_

						if (IMAGE_REL_BASED == pu->type)
						{
							ULONG_PTR Target = *pru - (ULONG_PTR)hmod;

#ifdef _X86_
							if (map.InUSection(Target) || Target == pvShellEnd)
							{
								// target in .text$mn$cpp$u - ok
								continue;
							}
#endif // _X86_
							char buf1[0x100], buf2[0x100];

							if (map.IsInit())
							{
								ULONG d1, d2;
								PCSTR pcszTarget;
								PCSTR name1 = unDNameEx(map.GetName(rva, &d1), buf1, _countof(buf1));
								PCSTR name2 = unDNameEx(pcszTarget = map.GetName(Target, &d2), buf2, _countof(buf2));

								if (d2) _UNLIKELY
								{
									if (d1) 
									{
										DbgPrint("!! Relocs: %08x ( %hs+%u ) -> %08x ( %hs+%u )\r\n",
											rva, name1, d1, Target, name2, d2);
									}
									else _UNLIKELY
									{
										DbgPrint("!! Relocs: %08x ( %hs ) -> %08x ( %hs+%u )\r\n",
											rva, name1, Target, name2, d2);
									}
								}
								else 
								{
									if (d1) 
									{
#ifdef _X86_
										if (strcmp(pcszTarget, "?__Address@@YIPAXPBX@Z")) _LIKELY
#endif // _X86_										
										{
											DbgPrint("!! Relocs: %08x ( %hs+%u ) -> %08x ( %hs )\r\n",
												rva, name1, d1, Target, name2);
										}
									}
									else _UNLIKELY
									{
										DbgPrint("!! Relocs: %08x ( %hs ) -> %08x ( %hs )\r\n",
											rva, name1, Target, name2);
									}
								}
							}
							else
							{
								DbgPrint("!! Relocs: %08x -> %08x\r\n", rva, Target);
							}

							if (pvShellEnd < Target)
							{
								DbgPrint("reloc to %08x beyond shell end ( %08x )\r\n", Target, pvShellEnd);
								return STATUS_ILLEGAL_DLL_RELOCATION;
							}

							if (!map.IsInit())
							{
								ULONG cch = _countof(buf1);
								if (CryptBinaryToStringA((PBYTE)*pru, 16, CRYPT_STRING_HEXASCII, buf1, &cch))
								{
									DbgPrint("\t%hs\r\n", buf1);
								}
							}
						}
#ifdef _X86_						
						else // if (IMAGE_REL_BASED_HIGHLOW == pu->type)
#endif // _X86_
						{
							DbgPrint("######## !! Exist Relocs (%x:%08x) !! ########\r\n", pu->type, rva);
							return STATUS_ILLEGAL_DLL_RELOCATION;
						}
					} // if (VirtualAddress + pu->ofs < pvShellEnd)
				} // if (pu->type)

			} while (pu++, --SizeOfBlock);

		} while (s);
	}

	DbgPrint("!! NO IMPORT, NO RELOCS. OK !!\r\n");

	return STATUS_SUCCESS;
}

NTSTATUS I_CreateAsmSC(PCWSTR pwzFileName, const void* pcv, SIZE_T cb)
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

		DbgPrint("CreateAsmSC(\"%ws\")=%x\r\n", pwzFileName, status);

		return status;
	}

	return STATUS_NO_MEMORY;
}

inline ULONG BOOL_TO_ERROR(BOOL f)
{
	return f ? NOERROR : GetLastError();
}

//## -> #
//#. -> *
//#: -> %

BOOL UnEscape(_Inout_ PWSTR str)
{
	PWSTR buf = str;
	WCHAR c;
	do
	{
		if ('#' == (c = *str++))
		{
			switch (c = *str++)
			{
			case '.':
				c = '*';
				break;
			case ':':
				c = '%';
				break;
			case '#':
				break;
			default:
				return FALSE;
			}
		}

		*buf++ = c;

	} while (c);

	return TRUE;
}

NTSTATUS CreateAesKey(_Out_ BCRYPT_KEY_HANDLE* phKey, _In_ PBYTE secret, _In_ ULONG cb)
{
	NTSTATUS status;
	BCRYPT_ALG_HANDLE hAlgorithm;
	if (0 <= (status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, 0, 0)))
	{
		status = BCryptGenerateSymmetricKey(hAlgorithm, phKey, 0, 0, secret, cb, 0);

		BCryptCloseAlgorithmProvider(hAlgorithm, 0);
	}

	return status;
}

NTSTATUS CreateAsmSC(PCWSTR to, PVOID pv, ULONG cb)
{
	if (PWSTR psz = wcsrchr(to, '?'))
	{
		*psz++ = 0;

		//while (!IsDebuggerPresent()) Sleep(100); __debugbreak();
		if (!UnEscape(const_cast<PWSTR>(to)))
		{
			return STATUS_BAD_DATA;
		}
		DbgPrint("password: \"%ws\"\r\n", to);

		ULONG len = RtlPointerToOffset(to, psz);
		NTSTATUS status = RtlUnicodeToUTF8N((char*)to, len, &len, to, len);

		if (0 > status)
		{
			return status;
		}

		BCRYPT_KEY_HANDLE hKey;
		UCHAR secret[32];
		ULONG s = sizeof(secret);
		if (CryptHashCertificate2(BCRYPT_SHA256_ALGORITHM, 0, 0, (PBYTE)to, len, secret, &s))
		{
			if (0 <= (status = CreateAesKey(&hKey, secret, s)))
			{
				PBYTE pb = 0;
				s = 0;
				while (0 <= (status = BCryptEncrypt(hKey, (PBYTE)pv, cb, 0, 0, 0, pb, s, &s, BCRYPT_BLOCK_PADDING)))
				{
					if (pb)
					{
						DbgPrint("Encrypt: %x -> %x\r\n", cb, s);
						status = I_CreateAsmSC(psz, pb, s);
						break;
					}

					if (!(pb = new UCHAR[s]))
					{
						status = STATUS_NO_MEMORY;
						break;
					}
				}

				if (pb)
				{
					delete[] pb;
				}

				BCryptDestroyKey(hKey);
			}

			return status;
		}

		return GetLastError();
	}

	return I_CreateAsmSC(to, pv, cb);
}

NTSTATUS CreateZipAsmSC(PCWSTR to, PVOID pv, ULONG cb)
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
					DbgPrint("Compress:%x >> %x [%u%%]\r\n", cb, CompressedDataSize, (CompressedDataSize * 100) / cb);
					dwError = CreateAsmSC(to, pb, (ULONG)CompressedDataSize);
				}
				else
				{
					dwError = GetLastError();
				}

				delete[] pb;
			}

			break;
		}

		CloseCompressor(CompressorHandle);

		return HRESULT_FROM_WIN32(dwError);
	}

	return GetLastError();
}

NTSTATUS CreateExeSC(PCWSTR pwzFileName, PVOID Base, ULONG cb, PVOID ImageBase)
{
	struct PE
	{
		union {
			UCHAR pad[0x200];
			struct
			{
				IMAGE_DOS_HEADER idh;
				IMAGE_NT_HEADERS inth;
				IMAGE_SECTION_HEADER ish;
			};
		};
		UCHAR text[];

		void* operator new(size_t s, ULONG cb)
		{
			return RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, s + cb);
		}

		void operator delete(void* p)
		{
			RtlFreeHeap(GetProcessHeap(), 0, p);
		}
	};

	if (PE* pe = new(cb) PE)
	{
		PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(ImageBase);

		memcpy(&pe->idh, ImageBase, sizeof(IMAGE_DOS_HEADER));
		memcpy(&pe->inth, pinth, sizeof(IMAGE_NT_HEADERS));

		pe->idh.e_lfanew = sizeof(IMAGE_DOS_HEADER);
		pe->inth.FileHeader.NumberOfSections = 1;
		pe->inth.FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
		pe->inth.FileHeader.Characteristics &= ~IMAGE_FILE_DLL;

		pe->inth.OptionalHeader.AddressOfEntryPoint = 0x1000;
		pe->inth.OptionalHeader.SizeOfCode = cb;
		pe->inth.OptionalHeader.BaseOfCode = 0x1000;
		pe->inth.OptionalHeader.SizeOfInitializedData = 0;
		pe->inth.OptionalHeader.SizeOfUninitializedData = 0;
		pe->inth.OptionalHeader.SectionAlignment = 0x1000;
		pe->inth.OptionalHeader.FileAlignment = 0x200;
		pe->inth.OptionalHeader.SizeOfImage = ((cb + 0x1FFF) & ~0xFFF);
		pe->inth.OptionalHeader.SizeOfHeaders = sizeof(PE);
		pe->inth.OptionalHeader.CheckSum = 0;
		pe->inth.OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		RtlZeroMemory(pe->inth.OptionalHeader.DataDirectory, sizeof(pe->inth.OptionalHeader.DataDirectory));

		memcpy(pe->ish.Name, ".text\0\0", IMAGE_SIZEOF_SHORT_NAME);
		pe->ish.Misc.VirtualSize = cb;
		pe->ish.VirtualAddress = 0x1000;
		pe->ish.SizeOfRawData = cb;
		pe->ish.PointerToRawData = sizeof(PE);
		pe->ish.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;

		if ('?' == *pwzFileName)
		{
			++pwzFileName;
			pe->ish.Characteristics |= IMAGE_SCN_MEM_WRITE;
		}

		memcpy(pe->text, Base, cb);

		NTSTATUS status = SaveToFile(pwzFileName, pe, sizeof(PE) + cb);

		delete pe;

		DbgPrint("CreateExeSC(\"%ws\")=%x\r\n", pwzFileName, status);

		return status;
	}

	return STATUS_NO_MEMORY;
}

NTSTATUS NTAPI PrepareSC(PVOID Base, ULONG cb, PVOID ImageBase)
{
	if (IsDebuggerPresent())
	{
		if (wcschr(GetCommandLineW(), '?'))
		{
			VirtualProtect(Base, cb, PAGE_EXECUTE_READWRITE, &cb);
		}

		return 0;
	}

	DbgPrint("PrepareSC(%p, %x, <%ws>)\r\n", Base, cb, GetCommandLineW());

	//*map*obj*imp*[bin]*[?password?asm][*?exe]

	if (PWSTR psz = wcschr(GetCommandLineW(), '*'))
	{
		PWSTR pczMap = ++psz;

		if (psz = wcschr(psz, '*'))
		{
			*psz++ = 0;

			PWSTR pczObj = psz;

			if (psz = wcschr(psz, '*'))
			{
				*psz++ = 0;

				PWSTR pczImp = psz, pczBin = 0, pczAsm = 0, pczExe = 0;

				if (psz = wcschr(psz, '*'))
				{
					*psz++ = 0;

					pczBin = psz;

					if (psz = wcschr(psz, '*'))
					{
						*psz++ = 0;

						pczAsm = psz;

						if (psz = wcschr(psz, '*'))
						{
							*psz++ = 0;

							pczExe = psz;
						}
					}
				}

				NTSTATUS status = ProcessIAT(ImageBase, pczObj, pczImp, pczMap, (ULONG_PTR)Base + cb);

				if (0 <= status)
				{
					if (pczBin && *pczBin)
					{
						status = SaveToFile(pczBin, Base, cb);
						DbgPrint("CreateBin(\"%ws\" [%x])=%x\r\n", pczBin, cb, status);
					}

					if (0 <= status)
					{
						if (pczAsm && *pczAsm)
						{
							status = '?' == *pczAsm ? CreateZipAsmSC(pczAsm + 1, Base, cb) : CreateAsmSC(pczAsm, Base, cb);
						}

						if (0 <= status)
						{
							if (pczExe && *pczExe)
							{
								status = CreateExeSC(pczExe, Base, cb, ImageBase);
							}
						}
					}
				}

				return PrintError(status);
			}
		}
	}

	return PrintError(STATUS_INVALID_PARAMETER);
}
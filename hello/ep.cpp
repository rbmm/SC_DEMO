#include "stdafx.h"

//#define _PRINT_CPP_NAMES_
#include "../ScEntry/address.h"

EXTERN_C
NTSYSAPI
VOID
NTAPI
RtlDispatchAPC(
    PAPCFUNC pfnAPC,
    ULONG_PTR dwData,
    PVOID ApcActivationContext
	);

EXTERN_C
WINBASEAPI
PCSTR
FASTCALL
DnsStatusString(_In_ DNS_STATUS status);

EXTERN_C
WINBASEAPI
DNS_STATUS
FASTCALL
DnsMapRcodeToStatus(_In_ UCHAR ResponseCode); // Sent in the "ResponseCode" field of a DNS_HEADER.

struct _ACC_MARTA_FUNCTIONS {};

WINBASEAPI
_ACC_MARTA_FUNCTIONS*
NTAPI
GetMartaExtensionInterface();

int MsgBox(UINT uType, PCWSTR lpCaption, PCWSTR format, ...)
{
	va_list args;
	va_start(args, format);

	int len = 0;
	PWSTR psz = 0;

	while (0 < (len = _vsnwprintf(psz, len, format, args)))
	{
		if (psz)
		{
			return MessageBoxW(0, psz, lpCaption, uType);
		}

		psz = (PWSTR)alloca(++len * sizeof(WCHAR));
	}

	return 0;
}

VOID NTAPI ApcTest(_In_ ULONG_PTR Parameter)
{
	DbgPrint(_YA("ApcTest(%p)\r\n"), Parameter);

	if (PCSTR pcsz = DnsStatusString(DnsMapRcodeToStatus(DNS_RCODE_YXDOMAIN)))
	{
		MsgBox(MB_ICONINFORMATION, _YW(L"Hello"), _YW(L"%u:[%hs]\r\nentry at %p"), GetTickCount(), pcsz, (PVOID)Parameter);
	}
}

void ComTest()
{
	HRESULT hr;
	if (0 <= (hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		PWSTR pszFilePath = 0;
		IFileOpenDialog* pFileOpen;

		if (0 <= (hr = CoCreateInstance(__uuidof(FileOpenDialog), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOpen))))
		{
			if (0 <= (hr = pFileOpen->SetOptions(FOS_PICKFOLDERS | FOS_NOVALIDATE | FOS_NOTESTFILECREATE | FOS_DONTADDTORECENT | FOS_FORCESHOWHIDDEN)) &&
				0 <= (hr = pFileOpen->Show(0)))
			{
				IShellItem* pItem;

				if (0 <= (hr = pFileOpen->GetResult(&pItem)))
				{
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					pItem->Release();
				}
			}

			pFileOpen->Release();

			if (pszFilePath)
			{
				MessageBoxW(0, pszFilePath, _YW(L"Com"), MB_ICONINFORMATION);
				CoTaskMemFree(pszFilePath);
			}
		}

		CoUninitialize();
	}

	if (hr)
	{
		wchar_t msg[0x100];
		if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, hr, 0, msg, _countof(msg), 0))
		{
			MessageBoxW(0, msg, _YW(L"Com"), MB_ICONINFORMATION);
		}
	}
}

void WINAPI ep()
{
	CPP_FUNCTION;

	printf(_YA("t = %u\r\n"), GetTickCount());
	
	BOOL Critical;
	IsProcessCritical(NtCurrentProcess(), &Critical);

	ComTest();

	if (_ACC_MARTA_FUNCTIONS* pTable = GetMartaExtensionInterface())
	{
		MsgBox(MB_ICONWARNING, _YW(L"Marta:"), _YW(L"table at %p"), pTable);
	}

	RtlDispatchAPC(_Y(ApcTest), (ULONG_PTR)_Y(ep), INVALID_HANDLE_VALUE);

	ExitProcess(0);
}
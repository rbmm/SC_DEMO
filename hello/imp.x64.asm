createFunc dnsapi, DnsMapRcodeToStatus, 0e44e2835h
createFunc dnsapi, DnsStatusString, 0bfe22618h

HMOD dnsapi, <DNSAPI.dll>

createFunc kernel32, IsProcessCritical, 039cba57ah
createFunc kernel32, FormatMessageW, 0b8c2559fh
createFunc kernel32, GetTickCount, 0bb509c20h

HMOD kernel32, <KERNEL32.dll>

createFunc ntmarta, ?GetMartaExtensionInterface@@YAPEAU_ACC_MARTA_FUNCTIONS@@XZ, 084c5f6c1h

HMOD ntmarta, <NTMARTA.dll>

createFunc user32, MessageBoxW, 0dd240029h

HMOD user32, <USER32.dll>

createFunc msvcrt, _vsnwprintf, 09513c274h
createFunc msvcrt, printf, 0fd702f77h

HMOD msvcrt, <msvcrt.dll>

createFunc ntdllp, DbgPrint, 0d6648df0h
createFunc ntdllp, RtlDispatchAPC, 080000008h
createFunc ntdllp, LdrGetProcedureAddress, 041f9a797h
createFunc ntdllp, RtlInitAnsiString, 00d40a150h
createFunc ntdllp, RtlMultiByteToUnicodeN, 0af775063h
createFunc ntdllp, LdrLoadDll, 0ee9958d8h
createFunc ntdllp, RtlInitUnicodeString, 066df655ah
createFunc ntdllp, RtlMultiByteToUnicodeSize, 0bc3ad488h

HMOD ntdllp, <>

createFunc ole32, CoTaskMemFree, 07026a250h
createFunc ole32, CoCreateInstance, 0cbe27501h
createFunc ole32, CoInitializeEx, 085535619h
createFunc ole32, CoUninitialize, 0d134d0bfh

HMOD ole32, <ole32.dll>


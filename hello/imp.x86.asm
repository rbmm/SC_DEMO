createFunc dnsapi, @DnsMapRcodeToStatus@4, 0e44e2835h
createFunc dnsapi, @DnsStatusString@4, 0bfe22618h

HMOD dnsapi, <DNSAPI.dll>

createFunc kernel32, _FormatMessageW@28, 0b8c2559fh
createFunc kernel32, _IsProcessCritical@8, 039cba57ah
createFunc kernel32, _ExitProcess@4, 054dfc7ebh

HMOD kernel32, <KERNEL32.dll>

createFunc ntmarta, ?GetMartaExtensionInterface@@YGPAU_ACC_MARTA_FUNCTIONS@@XZ, 084c5f6c1h

HMOD ntmarta, <NTMARTA.dll>

createFunc user32, _MessageBoxW@16, 0dd240029h

HMOD user32, <USER32.dll>

createFunc ntdllp, _DbgPrint, 0d6648df0h
createFunc ntdllp, _RtlDispatchAPC@12, 080000008h
createFunc ntdllp, __vsnwprintf, 09513c274h
createFunc ntdllp, _LdrGetProcedureAddress@16, 041f9a797h
createFunc ntdllp, _RtlInitAnsiString@8, 00d40a150h
createFunc ntdllp, _RtlMultiByteToUnicodeN@20, 0af775063h
createFunc ntdllp, _RtlMultiByteToUnicodeSize@12, 0bc3ad488h
createFunc ntdllp, _LdrLoadDll@16, 0ee9958d8h
createFunc ntdllp, _RtlInitUnicodeString@8, 066df655ah

HMOD ntdllp, <>

createFunc ole32, _CoCreateInstance@20, 0cbe27501h
createFunc ole32, _CoInitializeEx@8, 085535619h
createFunc ole32, _CoTaskMemFree@4, 07026a250h
createFunc ole32, _CoUninitialize@0, 0d134d0bfh

HMOD ole32, <ole32.dll>


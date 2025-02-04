createFunc dnsapi, DnsMapRcodeToStatus
createFunc dnsapi, DnsStatusString

HMOD dnsapi, <DNSAPI.dll>

createFunc kernel32, GetTickCount64
createFunc kernel32, FormatMessageW
createFunc kernel32, ExitProcess

HMOD kernel32, <KERNEL32.dll>

createFunc? ntmarta, ?GetMartaExtensionInterface@@YAPEAU_ACC_MARTA_FUNCTIONS@@XZ, 'GetMartaExtensionInterface'

HMOD ntmarta, <NTMARTA.dll>

createFunc user32, MessageBoxW

HMOD user32, <USER32.dll>

createFunc ntdllp, _vsnwprintf
createFunc ntdllp, DbgPrint
createFunc? ntdllp, RtlDispatchAPC, '#8'
createFunc ntdllp, LdrGetProcedureAddress
createFunc ntdllp, LdrLoadDll
createFunc ntdllp, RtlInitUnicodeString

HMOD ntdllp, <>

createFunc ole32, CoCreateInstance
createFunc ole32, CoInitializeEx
createFunc ole32, CoTaskMemFree
createFunc ole32, CoUninitialize

HMOD ole32, <ole32.dll>


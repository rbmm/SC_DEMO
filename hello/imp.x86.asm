createFuncF dnsapi, DnsMapRcodeToStatus, 4
createFuncF dnsapi, DnsStatusString, 4

HMOD dnsapi, <DNSAPI.dll>

createFuncS kernel32, GetTickCount, 0
createFuncS kernel32, FormatMessageW, 28
createFuncS kernel32, ExitProcess, 4

HMOD kernel32, <KERNEL32.dll>

createFunc? ntmarta, ?GetMartaExtensionInterface@@YGPAU_ACC_MARTA_FUNCTIONS@@XZ, 'GetMartaExtensionInterface'

HMOD ntmarta, <NTMARTA.dll>

createFuncS user32, MessageBoxW, 16

HMOD user32, <USER32.dll>

createFuncC ntdllp, _vsnwprintf
createFuncC ntdllp, DbgPrint
createFunc? ntdllp, _RtlDispatchAPC@12, '#8'
createFuncS ntdllp, LdrGetProcedureAddress, 16
createFuncS ntdllp, LdrLoadDll, 16
createFuncS ntdllp, RtlInitUnicodeString, 8

HMOD ntdllp, <>

createFuncS ole32, CoCreateInstance, 20
createFuncS ole32, CoInitializeEx, 8
createFuncS ole32, CoTaskMemFree, 4
createFuncS ole32, CoUninitialize, 0

HMOD ole32, <ole32.dll>


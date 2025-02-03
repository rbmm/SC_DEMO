#include "stdafx.h"

HANDLE GetProcessHeap()
{
	return NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;
}

void* __cdecl operator new[](size_t ByteSize)
{
	return RtlAllocateHeap(GetProcessHeap(), 0, ByteSize);
}

void* __cdecl operator new(size_t ByteSize)
{
	return RtlAllocateHeap(GetProcessHeap(), 0, ByteSize);
}

void __cdecl operator delete(void* Buffer)
{
	RtlFreeHeap(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete(void* Buffer, size_t)
{
	RtlFreeHeap(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete[](void* Buffer)
{
	RtlFreeHeap(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete[](void* Buffer, size_t)
{
	RtlFreeHeap(GetProcessHeap(), 0, Buffer);
}
#include "stdafx.h"

void* __cdecl operator new[](size_t ByteSize)
{
	return HeapAlloc(GetProcessHeap(), 0, ByteSize);
}

void* __cdecl operator new(size_t ByteSize)
{
	return HeapAlloc(GetProcessHeap(), 0, ByteSize);
}

void __cdecl operator delete(void* Buffer)
{
	HeapFree(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete(void* Buffer, size_t)
{
	HeapFree(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete[](void* Buffer)
{
	HeapFree(GetProcessHeap(), 0, Buffer);
}

void __cdecl operator delete[](void* Buffer, size_t)
{
	HeapFree(GetProcessHeap(), 0, Buffer);
}
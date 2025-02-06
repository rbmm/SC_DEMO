#pragma once

HRESULT Unzip(_In_ LPCVOID CompressedData,
			  _In_ ULONG CompressedDataSize,
			  _Out_ PVOID* pUncompressedBuffer,
			  _Out_ ULONG* pUncompressedDataSize);
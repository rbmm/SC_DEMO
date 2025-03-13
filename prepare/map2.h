#pragma once

struct MAP
{
	struct RO;

	PSTR _M_buf = 0;
	RO* _M_pr = 0;
	ULONG _M_n = 0, _M_rvaU = 0, _M_sizeU = 0, _M_minRVA = 0, _M_maxRVA = 0;

	~MAP()
	{
		if (_M_buf) delete [] _M_buf;
	}

	PCSTR GetName(_In_ ULONG_PTR rva, _Out_ ULONG* d);

	RO* Parse(PSTR pcsz, PIMAGE_NT_HEADERS pinth);

	BOOL Init(PCWSTR pszMap, PIMAGE_NT_HEADERS pinth);

	BOOL InUSection(ULONG Target)
	{
		return Target - _M_rvaU < _M_sizeU;
	}

	PSTR IsInit()
	{
		return _M_buf;
	}
};
/*
	Copyright (C) 2008-2009 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "types.h"
#include "FileMap.h"
#include <stdio.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/mman.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 
#endif

#ifdef _MSC_VER
class FileMap::Impl
{
public:
	Impl(const char *file);
	~Impl();

	bool Open(int size, bool del_on_close);
	void Close();

	void* GetPtr();
	int GetSize();

private:
	HANDLE m_hFile;
	HANDLE m_hFileMap;
	void *m_Ptr;
	int m_Size;

	char m_szFile[MAX_PATH];
};

FileMap::Impl::Impl(const char *file)
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_hFileMap(NULL)
	, m_Ptr(NULL)
	, m_Size(0)
{
	strncpy(m_szFile, file, MAX_PATH);
}

FileMap::Impl::~Impl()
{
	Close();
}

bool FileMap::Impl::Open(int size, bool del_on_close)
{
	Close();

	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_TEMPORARY;
	if (del_on_close)
		dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

	m_hFile = CreateFileA(m_szFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
						OPEN_ALWAYS, dwFlagsAndAttributes, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER file_szie;

	GetFileSizeEx(m_hFile, &file_szie);
	if (file_szie.QuadPart < size)
	{
		file_szie.QuadPart = size;
		SetFilePointerEx(m_hFile, file_szie, NULL, FILE_BEGIN);
		SetEndOfFile(m_hFile);
	}

	m_hFileMap = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (m_hFileMap == NULL)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;

		return false;
	}

	m_Size = size;
	m_Ptr = MapViewOfFile(m_hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (!m_Ptr)
	{
		CloseHandle(m_hFileMap);
		CloseHandle(m_hFile);

		m_hFileMap = NULL;
		m_hFile = INVALID_HANDLE_VALUE;
		m_Size = 0;
	}

	return true;
}

void FileMap::Impl::Close()
{
	if (m_Ptr)
	{
		UnmapViewOfFile(m_Ptr);
		CloseHandle(m_hFileMap);
		CloseHandle(m_hFile);

		m_Ptr = NULL;
		m_hFileMap = NULL;
		m_hFile = INVALID_HANDLE_VALUE;
		m_Size = 0;
	}
}

void* FileMap::Impl::GetPtr()
{
	return m_Ptr;
}

int FileMap::Impl::GetSize()
{
	return m_Size;
}
#else
class FileMap::Impl
{
public:
	Impl(const char *file);
	~Impl();

	bool Open(int size, bool del_on_close);
	void Close();

	void* GetPtr();
	int GetSize();

private:
	int m_hFile;
	void *m_Ptr;
	int m_Size;
	bool m_DelOnClose;

	char m_szFile[MAX_PATH];
};

FileMap::Impl::Impl(const char *file)
	: m_hFile(-1)
	, m_Ptr(NULL)
	, m_Size(0)
	, m_DelOnClose(false)
{
	strncpy(m_szFile, file, MAX_PATH);
}

FileMap::Impl::~Impl()
{
	Close();
}

bool FileMap::Impl::Open(int size, bool del_on_close)
{
	Close();

	m_hFile = open(m_szFile, O_RDWR | O_CREAT, S_IRWXU);
	if (m_hFile == -1)
		return false;

	//int file_szie = lseek(m_hFile, 0, SEEK_END);
	//if (file_szie < size)
	{
		char tmp = 0;

		lseek(m_hFile, size-1, SEEK_SET);
		write(m_hFile, &tmp, 1);
	}

	m_Size = size;
	m_DelOnClose = del_on_close;
	m_Ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_hFile, 0);
	if (m_Ptr == MAP_FAILED)
	{
		close(m_hFile);

		m_Ptr = NULL;
		m_hFile = -1;
		m_Size = 0;
		m_DelOnClose = false;
	}

	return true;
}

void FileMap::Impl::Close()
{
	if (m_Ptr)
	{
		munmap(m_Ptr, m_Size);
		close(m_hFile);

		if (m_DelOnClose)
			remove(m_szFile);

		m_Ptr = NULL;
		m_hFile = -1;
		m_Size = 0;
		m_DelOnClose = false;
	}
}

void* FileMap::Impl::GetPtr()
{
	return m_Ptr;
}

int FileMap::Impl::GetSize()
{
	return m_Size;
}
#endif

FileMap::FileMap(const char *file)
	: impl(new FileMap::Impl(file))
{
}

FileMap::~FileMap()
{
	delete impl;
}

bool FileMap::Open(int size, bool del_on_close)
{
	return impl->Open(size, del_on_close);
}

void FileMap::Close()
{
	impl->Close();
}

void* FileMap::GetPtr()
{
	return impl->GetPtr();
}

int FileMap::GetSize()
{
	return impl->GetSize();
}

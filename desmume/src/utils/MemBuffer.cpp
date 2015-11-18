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

#include "MemBuffer.h"
#include "../debug.h"
#include <stdio.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#endif

u32 MemBuffer::s_PageSize = 0;

static u32 CalcPages(u32 size, u32 pagesize)
{
	return (size + pagesize-4) / pagesize;
}

#ifdef _WINDOWS
static u32 GetPageSize()
{
	SYSTEM_INFO system_info;

	GetSystemInfo(&system_info);

	return system_info.dwPageSize;
}

static DWORD ConvertToWinApi(int mode)
{
	DWORD winmode = PAGE_NOACCESS;

	if (mode & MemBuffer::kExec)
	{
		winmode = (mode & MemBuffer::kWrite) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
	}
	else if (mode & MemBuffer::kRead)
	{
		winmode = (mode & MemBuffer::kWrite) ? PAGE_READWRITE : PAGE_READONLY;
	}

	return winmode;
}

u8* MemBuffer::Reserve(u32 size)
{
	if (m_Baseptr) return m_Baseptr;

	if (size < m_DefSize) size = m_DefSize;
	if (!size) return NULL;

	m_ReservedPages = CalcPages(size, s_PageSize);
	m_ReservedSize = m_ReservedPages * s_PageSize;
	m_CommittedSize = 0;

	m_Baseptr = (u8*)VirtualAlloc(NULL, m_ReservedSize, MEM_RESERVE, PAGE_NOACCESS);
	if (!m_Baseptr || !Commit(m_DefSize))
		Release();

	return m_Baseptr;
}

void MemBuffer::Release()
{
	if (m_Baseptr)
	{
		VirtualFree(m_Baseptr, 0, MEM_RELEASE);
		m_Baseptr = NULL;
	}

	m_ReservedSize = 0;
	m_ReservedPages = 0;
	m_CommittedSize = 0;
	m_UsedSize = 0;
}

bool MemBuffer::Commit(u32 size)
{
	if (!m_Baseptr) return false;

	if (size <= m_CommittedSize) return true;
	if (size > m_ReservedSize) return false;

	u32 pages = CalcPages(size, s_PageSize);
	size = pages * s_PageSize;

	u8* ptr = (u8*)VirtualAlloc(m_Baseptr, size, MEM_COMMIT, ConvertToWinApi(m_Mode));
	if (ptr)
	{
		m_CommittedSize = size;
		return true;
	}

	return false;
}
#else
static u32 GetPageSize()
{
	//return getpagesize();
	return (u32)sysconf(_SC_PAGESIZE);
}

static int ConvertToLnxApi(int mode)
{
	int lnxmode = 0;

	if (mode & MemBuffer::kExec)
		lnxmode |= PROT_EXEC | PROT_READ;
	if (mode & MemBuffer::kRead)
		lnxmode |= PROT_READ;
	if (mode & MemBuffer::kWrite)
		lnxmode |= PROT_WRITE;

	return lnxmode;
}

u8* MemBuffer::Reserve(u32 size)
{
	if (m_Baseptr) return m_Baseptr;

	if (size < m_DefSize) size = m_DefSize;
	if (!size) return NULL;

	m_ReservedPages = CalcPages(size, s_PageSize);
	m_ReservedSize = m_ReservedPages * s_PageSize;
	m_CommittedSize = 0;

	m_Baseptr = (u8*)mmap(NULL, m_ReservedSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (m_Baseptr == MAP_FAILED || !Commit(m_DefSize))
		Release();

	return m_Baseptr;
}

void MemBuffer::Release()
{
	if (m_Baseptr)
	{
		munmap(m_Baseptr, m_ReservedSize);
		m_Baseptr = NULL;
	}

	m_ReservedSize = 0;
	m_ReservedPages = 0;
	m_CommittedSize = 0;
	m_UsedSize = 0;
}

bool MemBuffer::Commit(u32 size)
{
	if (!m_Baseptr) return false;

	if (size <= m_CommittedSize) return true;
	if (size > m_ReservedSize) return false;

	u32 pages = CalcPages(size, s_PageSize);
	size = pages * s_PageSize;

	int err = mprotect(m_Baseptr, size, ConvertToLnxApi(m_Mode));
	if (err == 0)
	{
		m_CommittedSize = size;
		return true;
	}

	return false;
}
#endif

MemBuffer::MemBuffer(u32 mode, u32 def_size)
	: m_Baseptr(NULL)

	, m_Mode(mode)

	, m_DefSize(def_size)

	, m_ReservedSize(0)
	, m_ReservedPages(0)

	, m_CommittedSize(0)
	, m_UsedSize(0)
{
	if (!s_PageSize)
	{
		s_PageSize = GetPageSize();
		INFO("PageSize : %u\n", s_PageSize);
	}
}

MemBuffer::~MemBuffer()
{
	Release();
}

u8* MemBuffer::Alloc(u32 size)
{
	u8* ptr = NULL;
	u32 off = size + m_UsedSize;

	if (off <= m_CommittedSize || Commit(off))
	{
		ptr = m_Baseptr + m_UsedSize;
		m_UsedSize = off;
	}

	return ptr;
}

void MemBuffer::Free(u32 size)
{
	if (m_UsedSize >= size)
		m_UsedSize -= size;
	else
		m_UsedSize = 0;
}

void MemBuffer::Reset()
{
	m_UsedSize = 0;
}

u8* MemBuffer::GetBasePtr()
{
	return m_Baseptr;
}

u32 MemBuffer::GetReservedSize()
{
	return m_ReservedSize;
}

u32 MemBuffer::GetCommittedSize()
{
	return m_CommittedSize;
}

u32 MemBuffer::GetUsedSize()
{
	return m_UsedSize;
}

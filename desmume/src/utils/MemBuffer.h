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

#ifndef _MEMBUFFER_H_
#define _MEMBUFFER_H_

#include "../types.h"

class MemBuffer
{
public:
	enum Mode
	{
		kRead =  1<<0,
		kWrite = 1<<1,
		kExec =  1<<2,
	};

public:
	MemBuffer(u32 mode, u32 def_size);

	~MemBuffer();

	u8* Reserve(u32 size = 0);

	void Release();

	u8* Alloc(u32 size);

	void Free(u32 size);

	void Reset();

	u8* GetBasePtr();

	u32 GetReservedSize();

	u32 GetCommittedSize();

	u32 GetUsedSize();

protected:
	bool Commit(u32 size);

private:
	u8* m_Baseptr;

	u32 m_Mode;

	u32 m_DefSize;

	u32 m_ReservedSize;
	u32 m_ReservedPages;

	u32 m_CommittedSize;
	u32 m_UsedSize;

	static u32 s_PageSize;
};

#endif

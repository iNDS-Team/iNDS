/*
	Copyright (C) 2008-2010 DeSmuME team

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

#ifndef CPU_BASE
#define CPU_BASE

#include "common.h"

#define CPUBASE_FLUSHALL ((u32)-1)

struct CpuBase
{
	void (*Reserve)();

	void (*Shutdown)();

	void (*Reset)();

	void (*Sync)();

	void (*Clear[2])(u32 Addr, u32 Size);

	u32 (*Execute[2])();

	u32 (*GetCacheReserve)();
	void (*SetCacheReserve)(u32 reserveInMegs);

	const char* (*Description)();
};

extern CpuBase *arm_cpubase;

#endif

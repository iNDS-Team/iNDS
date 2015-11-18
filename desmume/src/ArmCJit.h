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
#ifndef ARM_CJIT
#define ARM_CJIT

#include "common.h"
#include "CpuBase.h"
#include "JitCommon.h"
#include "armcpu.h"

typedef u32 (* ArmOpCompiled)();

template<unsigned int PROCNUM> u32 armcpu_compileCJIT();

template<unsigned int PROCNUM> u32 cpuExecuteCJIT()
{
	ArmOpCompiled opfun = (ArmOpCompiled)JITLUT_HANDLE(ARMPROC.instruct_adr, PROCNUM);
	if (opfun)
		return opfun();

	return armcpu_compileCJIT<PROCNUM>();
}

extern CpuBase arm_cjit;

#endif
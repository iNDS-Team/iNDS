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

#ifndef ARM_THREADEDINTERPRETER
#define ARM_THREADEDINTERPRETER

#include "common.h"
#include "CpuBase.h"
#include "armcpu.h"

#include "ArmThreadedInterpreter.h"
#include "ArmAnalyze.h"
#include "JitCommon.h"

typedef u32 (FASTCALL* OpCompiler)(const Decoded& d, struct MethodCommon* common);
typedef void (FASTCALL* OpMethod)(const struct MethodCommon* common);

struct MethodCommon
{
	OpMethod func;
	void* data;
	u32 R15;
};


struct Block
{
	MethodCommon *ops;
	static u32 cycles;
};

template<int PROCNUM> static Block* armcpu_compileblock(BlockInfo &blockinfo);

template<int PROCNUM> Block* armcpu_compile();

template<int PROCNUM> static u32 cpuExecute()
{
	Block *block = (Block*)JITLUT_HANDLE(ARMPROC.instruct_adr, PROCNUM);
	if (!block)
        block = armcpu_compile<PROCNUM>();

#ifdef DUMPLOG
	extern unsigned long long RawGetTickCount();

	unsigned long long start = RawGetTickCount();
#endif

	block->cycles = 0;
	block->ops->func(block->ops);

#ifdef DUMPLOG
	u32 time = (u32)(RawGetTickCount() - start);

	std::map<u32,EInfo>::iterator itr = exec_info[PROCNUM].find(ARMPROC.instruct_adr);
	if (itr != exec_info[PROCNUM].end())
	{
		itr->second.count++;
		itr->second.time += time;
	}
	else
	{
		EInfo info;
		info.count = 1;
		info.time = time;

		exec_info[PROCNUM][ARMPROC.instruct_adr] = info;
	}
#endif
    //printf("Cycles: %d\n", block->cycles);
	return block->cycles;
}

extern CpuBase arm_threadedinterpreter;

#endif

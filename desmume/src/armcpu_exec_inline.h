/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2012 DeSmuME team

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

#ifndef _ARMCPU_EXEC_INLINE_H
#define _ARMCPU_EXEC_INLINE_H

#include "arm_instructions.h"
#include "thumb_instructions.h"
#include "MMU_timing.h"
#include "ArmThreadedInterpreter.h"
#include "ArmLJit.h"

template<u32> static void armcpu_prefetch();

template<u32 PROCNUM>
static void armcpu_prefetch()
{
	u32 curInstruction = ARMPROC.next_instruction;

	if(ARMPROC.CPSR.bits.T == 0)
	{

		curInstruction &= 0xFFFFFFFC; //please don't change this to 0x0FFFFFFC -- the NDS will happily run on 0xF******* addresses all day long
		//please note that we must setup R[15] before reading the instruction since there is a protection
		//which prevents PC > 0x3FFF from reading the bios region
		ARMPROC.instruct_adr = curInstruction;
		ARMPROC.next_instruction = curInstruction + 4;
		ARMPROC.R[15] = curInstruction + 8;
		ARMPROC.instruction = _MMU_read32<PROCNUM, MMU_AT_CODE>(curInstruction);

		MMU_codeFetchCycles<PROCNUM,32>(curInstruction);
		return;
	}


	curInstruction &= 0xFFFFFFFE; //please don't change this to 0x0FFFFFFE -- the NDS will happily run on 0xF******* addresses all day long
	//please note that we must setup R[15] before reading the instruction since there is a protection
	//which prevents PC > 0x3FFF from reading the bios region
	ARMPROC.instruct_adr = curInstruction;
	ARMPROC.next_instruction = curInstruction + 2;
	ARMPROC.R[15] = curInstruction + 4;
	ARMPROC.instruction = _MMU_read16<PROCNUM, MMU_AT_CODE>(curInstruction);


	if(PROCNUM==0)
	{
		// arm9 fetches 2 instructions at a time in thumb mode
		if(!(curInstruction == ARMPROC.instruct_adr + 2 && (curInstruction & 2)))
			MMU_codeFetchCycles<PROCNUM,32>(curInstruction);
		return;
	}

	MMU_codeFetchCycles<PROCNUM,16>(curInstruction);
	return;
}

template<int PROCNUM> u32 armcpu_exec()
{
	// Usually, fetching and executing are processed parallelly.
	// So this function stores the cycles of each process to
	// the variables below, and returns appropriate cycle count.
	u32 cExecute;

	if(ARMPROC.CPSR.bits.T == 0)
	{
		const u32 condition = CONDITION(ARMPROC.instruction); 
		if(
			condition == 0x0E  //fast path for unconditional instructions
			|| (TEST_COND(condition, CODE(ARMPROC.instruction), ARMPROC.CPSR)) //handles any condition
			)
		{

			cExecute = arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(ARMPROC.instruction)](ARMPROC.instruction);
		}
		else
			cExecute = 1; // If condition=false: 1S cycle

		armcpu_prefetch<PROCNUM>();
		//return MMU_fetchExecuteCycles<PROCNUM>(cExecute, cFetch);
		return cExecute;
	}

	cExecute = thumb_instructions_set[PROCNUM][ARMPROC.instruction>>6](ARMPROC.instruction);
	
	armcpu_prefetch<PROCNUM>();
	//return MMU_fetchExecuteCycles<PROCNUM>(cExecute, cFetch);
	return cExecute;
}



template<int PROCNUM, int cpuMode>
u32 armcpu_exec()
{
	if(cpuMode == 1)
		return cpuExecute<PROCNUM>();
	else if(cpuMode == 0 || arm_cpubase == NULL)
		return armcpu_exec<PROCNUM>();
	return arm_cpubase->Execute[PROCNUM]();
}


#endif
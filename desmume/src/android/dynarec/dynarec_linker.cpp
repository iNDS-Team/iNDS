/*
 * Copyright (C) 2012 Qingping He
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "dynarec_linker.h"
#include "MMU.h"
#include "cp15.h"
extern "C"
{


u32 dynarec_proc = ARMCPU_ARM9;
armcpu_t* dynarec_cpu;




u32 ds_read32(u32 b)
{
	if(dynarec_proc == 0)
	{
		if((b & 0xFFFFC000ul) == MMU.DTCMRegion)
			return *(u32*)(MMU.ARM9_DTCM + (b & 0x3FFCul));
		if ( (b & 0x0F000000ul) == 0x02000000ul)
			return *(u32*)(MMU.MAIN_MEM + (b & 0x3FFFFCul));
		return _MMU_ARM9_read32(b);
	}
	else
	{
		if ( (b & 0x0F000000ul) == 0x02000000ul)
			return *(u32*)(MMU.MAIN_MEM + (b & 0x3FFFFCul));
		return _MMU_ARM7_read32(b);
	}
	//return _MMU_read32(dynarec_proc, MMU_AT_DATA, (b) & 0xFFFFFFFC);
}

u16 ds_read16(u32 b)
{
	return _MMU_read16(dynarec_proc, MMU_AT_DATA, (b) & 0xFFFFFFFE);
}

u8 ds_read8(u32 b)
{
	return _MMU_read08(dynarec_proc, MMU_AT_DATA, b);
}

u32 read32_cycles(u32 adr)
{
	//return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7,32,MMU_AD_READ>(3,adr) : MMU_aluMemAccessCycles<ARMCPU_ARM9,32,MMU_AD_READ>(3,adr);
	return dynarec_proc ? 2 : 4;
}

u32 read16_cycles(u32 adr)
{
	return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7,16,MMU_AD_READ>(3,adr) : MMU_aluMemAccessCycles<ARMCPU_ARM9,16,MMU_AD_READ>(3,adr);
}

u32 read8_cycles(u32 adr)
{
	return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7,8,MMU_AD_READ>(3,adr) : MMU_aluMemAccessCycles<ARMCPU_ARM9,8,MMU_AD_READ>(3,adr);
}


bool joke;
bool joker()
{
	return joke;
}
u32 ds_write32(u32 b, u32 c)
{
	write_smc_check(b);
	if(dynarec_proc == 0)
	{
		if((b & 0xFFFFC000ul) == MMU.DTCMRegion)
		{
			*(u32*)(MMU.ARM9_DTCM + (b & 0x3FFCul)) = c;
			//return MMU_aluMemAccessCycles<ARMCPU_ARM9, 32,MMU_AD_WRITE>(2,b); //figure these out?
			return 4;
		}
		if ( (b & 0x0F000000ul) == 0x02000000ul)
		{
			*(u32*)(MMU.MAIN_MEM + (b & 0x3FFFFCul)) = c;
			//return MMU_aluMemAccessCycles<ARMCPU_ARM9, 32,MMU_AD_WRITE>(2,b);
			return 4;
		}
		_MMU_ARM9_write32(b,c);
		//return MMU_aluMemAccessCycles<ARMCPU_ARM9, 32,MMU_AD_WRITE>(2,b);
		return 4;
	}
	else
	{
		if ( (b & 0x0F000000ul) == 0x02000000ul)
		{
			*(u32*)(MMU.MAIN_MEM + (b & 0x3FFFFCul)) = c;
			//return MMU_aluMemAccessCycles<ARMCPU_ARM7, 32,MMU_AD_WRITE>(2,b);
			return 4;
		}
		_MMU_ARM7_write32(b,c);
		//return MMU_aluMemAccessCycles<ARMCPU_ARM7, 32,MMU_AD_WRITE>(2,b);
		return 4;
	}
	_MMU_write32(dynarec_proc, MMU_AT_DATA, (b) & 0xFFFFFFFC,c);
	//return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7, 32,MMU_AD_WRITE>(2,b) : MMU_aluMemAccessCycles<ARMCPU_ARM9, 32,MMU_AD_WRITE>(2,b);
	return 4;
}

u32 ds_write16(u32 b, u16 c)
{
	_MMU_write16(dynarec_proc, MMU_AT_DATA, (b) & 0xFFFFFFFE,c);
	return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7, 16,MMU_AD_WRITE>(2,b) : MMU_aluMemAccessCycles<ARMCPU_ARM9, 16 ,MMU_AD_WRITE>(2,b);
}

u32 ds_write8(u32 b, u8 c)
{
	_MMU_write08(dynarec_proc, MMU_AT_DATA, b, c);
	return dynarec_proc ? MMU_aluMemAccessCycles<ARMCPU_ARM7, 8 ,MMU_AD_WRITE>(2,b) : MMU_aluMemAccessCycles<ARMCPU_ARM9, 8,MMU_AD_WRITE>(2,b);
}

void arm_mcr(u32 pc)
{
	u32 i=_MMU_read32(dynarec_proc, MMU_AT_CODE, pc-4);
	//store_dynarec_psr(&dynarec_cpu->CPSR, reg[Dynarec::REG_CPSR]);
	u32 cpnum = REG_POS(i, 8);

		if(cpnum !=15)
		{
			//LOGE("adr %x instruct %x cpnum %x", reg[Dynarec::REG_PC], i, cpnum);
		}
	/*if(!cpu->codynarec_proc[cpnum])
	{
		//emu_halt();
		//INFO("Stopped (OP_MCR) \n");
		INFO("ARM%c: MCR P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated codynarec_processor)\n",
			dynarec_procNUM?'7':'9', cpnum, REG_POS(i, 12), REG_POS(i, 16), REG_POS(i, 0), (i>>21)&0x7, (i>>5)&0x7);
		return 2;
	}*/

	armcp15_moveARM2CP(dynarec_cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i>>21)&0x7, (i>>5)&0x7);
	//cpu->codynarec_proc[cpnum]->moveARM2CP(cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i>>21)&7, (i>>5)&7);
	//return 2;
	//load_dynarec_psr(&dynarec_cpu->CPSR, &reg[Dynarec::REG_CPSR]);
}

void arm_mrc(u32 pc)
{
	u32 i=_MMU_read32(dynarec_proc, MMU_AT_CODE, pc-4);
	//if (dynarec_procNUM != 0) return 1;
	//store_dynarec_psr(&dynarec_cpu->CPSR, reg[Dynarec::REG_CPSR]);
	u32 cpnum = REG_POS(i, 8);


		//LOGE("adr %x instruct %x cpnum %x", reg[Dynarec::REG_PC], i, cpnum);

	/*if(!cpu->codynarec_proc[cpnum])
	{
		//emu_halt();
		//INFO("Stopped (OP_MRC) \n");
		INFO("ARM%c: MRC P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated codynarec_processor)\n",
			dynarec_procNUM?'7':'9', cpnum, REG_POS(i, 12), REG_POS(i, 16), REG_POS(i, 0), (i>>21)&0x7, (i>>5)&0x7);
		return 2;
	}*/

	// ARM REF:
	//data = value from Codynarec_processor[cp_num]
	//if Rd is R15 then
	//	N flag = data[31]
	//	Z flag = data[30]
	//	C flag = data[29]
	//	V flag = data[28]
	//else /* Rd is not R15 */
	//	Rd = data

	armcp15_moveCP2ARM(&dynarec_cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i>>21)&0x7, (i>>5)&0x7);

	if (REG_POS(i, 12) == 15)
	{
		dynarec_cpu->R[Dynarec::REG_CPSR] = dynarec_cpu->R[REG_POS(i, 12)];
		LOGE("oh no"); exit(0);
	}
	//cpu->codynarec_proc[cpnum]->moveCP2ARM(&cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i>>21)&7, (i>>5)&7);
	//load_dynarec_psr(&dynarec_cpu->CPSR, &reg[Dynarec::REG_CPSR]);
}

u32 arm_swi_exec(u32 pc)
{
	u32 i=_MMU_read32(dynarec_proc, MMU_AT_CODE, pc-4);
	LOGE("yoyo %x pc %x", i, pc-4);

	u32 swinum = (i>>16)&0xFF;

	//ideas-style debug prints (execute this SWI with the null terminated string address in R0)
	if(swinum==0xFC)
	{
		return 0;
	}

	//if the user has changed the intVector to point away from the nds bioses,
	//then it doesn't really make any sense to use the builtin SWI's since
	//the bios ones aren't getting called anyway
	bool bypassBuiltinSWI =
		(dynarec_cpu->intVector == 0x00000000 && dynarec_proc==0)
		|| (dynarec_cpu->intVector == 0xFFFF0000 && dynarec_proc==1);

	if(dynarec_cpu->swi_tab && !bypassBuiltinSWI)
	{
		swinum &= 0x1F;
		//printf("%d ARM SWI %d \n",dynarec_procNUM,swinum);
		u32 val=dynarec_cpu->swi_tab[swinum]() + 3;
		return val;
	}
	else
	{
		/* TODO (#1#): translocated SWI vectors */
		/* we use an irq thats not in the irq tab, as
		 it was replaced duie to a changed intVector */
		Status_Reg tmp;
		tmp.val = dynarec_cpu->R[16];
		armcpu_switchMode(dynarec_cpu, SVC);				/* enter svc mode */
		dynarec_cpu->R[14] = dynarec_cpu->R[15];
		dynarec_cpu->SPSR = tmp;							/* save old CPSR as new SPSR */
		dynarec_cpu->R[16] &= ~0x20;				/* handle as ARM32 code */
		dynarec_cpu->R[16] |= 0x80;
		dynarec_cpu->R[15] = dynarec_cpu->intVector + 0x08;
		dynarec_cpu->next_instruction = dynarec_cpu->R[15];
		return 4;
	}


}

u32 thumb_swi_exec(u32 pc)
{
	u32 i=_MMU_read16(dynarec_proc, MMU_AT_CODE, pc-2);
	//LOGE("yoyo %x", i);
	u32 swinum = i & 0xFF;

	//if the user has changed the intVector to point away from the nds bioses,
	//then it doesn't really make any sense to use the builtin SWI's since
	//the bios ones aren't getting called anyway
	bool bypassBuiltinSWI =
		(dynarec_cpu->intVector == 0x00000000 && dynarec_proc==0)
		|| (dynarec_cpu->intVector == 0xFFFF0000 && dynarec_proc==1);

	if(dynarec_cpu->swi_tab && !bypassBuiltinSWI) {
		 //zero 25-dec-2008 - in arm, we were masking to 0x1F.
		 //this is probably safer since an invalid opcode could crash the emu
		 //zero 30-jun-2009 - but they say that the ideas 0xFF should crash the device...
		 //u32 swinum = cpu->instruction & 0xFF;
		swinum &= 0x1F;
		//printf("%d ARM SWI %d\n",PROCNUM,swinum);
	   return dynarec_cpu->swi_tab[swinum]() + 3;
	}
	else {
	   /* we use an irq thats not in the irq tab, as
	   it was replaced due to a changed intVector */
	   Status_Reg tmp;
	   tmp.val = dynarec_cpu->R[16];
	   armcpu_switchMode(dynarec_cpu, SVC);		  /* enter svc mode */
	   dynarec_cpu->R[14] = dynarec_cpu->R[15];		  /* jump to swi Vector */
	   dynarec_cpu->SPSR = tmp;					/* save old CPSR as new SPSR */
	   dynarec_cpu->R[16] &= ~0x20;				/* handle as ARM32 code */
	   dynarec_cpu->R[16] |= 0x80;
	   dynarec_cpu->R[15] = dynarec_cpu->intVector + 0x08;
	   dynarec_cpu->next_instruction = dynarec_cpu->R[15];
	   return 3;
	}

}

extern void flush_page_table();

void dynarec_DeInit()
{
	flush_page_table();
	LOGE("page table flushed");
}

/*void dynarec_full_store()
{
	armcpu_switchMode(dynarec_cpu, reg[Dynarec::REG_CPSR] & 0x1F);
	store_dynarec_arm();

	dynarec_cpu->R13_usr = reg_mode[MODE_USER][5]; dynarec_cpu->R14_usr = reg_mode[MODE_USER][6];
	dynarec_cpu->R13_svc = reg_mode[MODE_SUPERVISOR][5]; dynarec_cpu->R14_svc = reg_mode[MODE_SUPERVISOR][6];
	dynarec_cpu->R13_abt = reg_mode[MODE_ABORT][5]; dynarec_cpu->R14_abt = reg_mode[MODE_ABORT][6];
	dynarec_cpu->R13_und = reg_mode[MODE_UNDEFINED][5]; dynarec_cpu->R14_und = reg_mode[MODE_UNDEFINED][6];
	dynarec_cpu->R13_irq = reg_mode[MODE_IRQ][5]; dynarec_cpu->R14_irq = reg_mode[MODE_IRQ][6];

	dynarec_cpu->R8_fiq = reg_mode[MODE_FIQ][0];
	dynarec_cpu->R9_fiq = reg_mode[MODE_FIQ][1];
	dynarec_cpu->R10_fiq = reg_mode[MODE_FIQ][2];
	dynarec_cpu->R11_fiq = reg_mode[MODE_FIQ][3];
	dynarec_cpu->R12_fiq = reg_mode[MODE_FIQ][4];
	dynarec_cpu->R13_fiq = reg_mode[MODE_FIQ][5];
	dynarec_cpu->R14_fiq = reg_mode[MODE_FIQ][6];

	store_dynarec_psr(&dynarec_cpu->SPSR_svc , spsr[MODE_SUPERVISOR]);
	store_dynarec_psr(&dynarec_cpu->SPSR_abt , spsr[MODE_ABORT]);
	store_dynarec_psr(&dynarec_cpu->SPSR_und , spsr[MODE_UNDEFINED]);
	store_dynarec_psr(&dynarec_cpu->SPSR_irq , spsr[MODE_IRQ]);
	store_dynarec_psr(&dynarec_cpu->SPSR_fiq , spsr[MODE_FIQ]);
}

void dynarec_full_load()
{
	set_cpu_mode(cpu_modes[dynarec_cpu->CPSR.bits.mode]);
	load_dynarec_arm();

	reg_mode[MODE_USER][5] = dynarec_cpu->R13_usr; reg_mode[MODE_USER][6] = dynarec_cpu->R14_usr;
	reg_mode[MODE_SUPERVISOR][5] = dynarec_cpu->R13_svc; reg_mode[MODE_SUPERVISOR][6] = dynarec_cpu->R14_svc;
	reg_mode[MODE_ABORT][5] = dynarec_cpu->R13_abt; reg_mode[MODE_ABORT][6] = dynarec_cpu->R14_abt;
	reg_mode[MODE_UNDEFINED][5] = dynarec_cpu->R13_und; reg_mode[MODE_UNDEFINED][6] = dynarec_cpu->R14_und;
	reg_mode[MODE_IRQ][5] = dynarec_cpu->R13_irq; reg_mode[MODE_IRQ][6] = dynarec_cpu->R14_irq;

	reg_mode[MODE_FIQ][0] = dynarec_cpu->R8_fiq;
	reg_mode[MODE_FIQ][1] = dynarec_cpu->R9_fiq;
	reg_mode[MODE_FIQ][2] = dynarec_cpu->R10_fiq;
	reg_mode[MODE_FIQ][3] = dynarec_cpu->R11_fiq ;
	reg_mode[MODE_FIQ][4] = dynarec_cpu->R12_fiq;
	reg_mode[MODE_FIQ][5] = dynarec_cpu->R13_fiq;
	reg_mode[MODE_FIQ][6] = dynarec_cpu->R14_fiq;

	load_dynarec_psr(&dynarec_cpu->SPSR_svc , &spsr[MODE_SUPERVISOR]);
	load_dynarec_psr(&dynarec_cpu->SPSR_abt , &spsr[MODE_ABORT]);
	load_dynarec_psr(&dynarec_cpu->SPSR_und , &spsr[MODE_UNDEFINED]);
	load_dynarec_psr(&dynarec_cpu->SPSR_irq , &spsr[MODE_IRQ]);
	load_dynarec_psr(&dynarec_cpu->SPSR_fiq , &spsr[MODE_FIQ]);
}*/

}

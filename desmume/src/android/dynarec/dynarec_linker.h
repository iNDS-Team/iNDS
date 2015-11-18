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

#ifndef DYNAREC_LINKER_H
#define DYNAREC_LINKER_H

#include "types.h"
#include "armcpu.h"
#include "dynarec/cpu.h"
#include "NDSSystem.h"
#include <android/log.h>
#include "../armcpu.h"
#include "bios.h"

extern "C"
{

extern u32* spsr_pointer;
extern u32 dynarec_proc;
extern armcpu_t* dynarec_cpu;

/*FORCEINLINE void switch_arm7()
{
	dynarec_proc = 1;
	dynarec_cpu = &NDS_ARM7;
	spsr_pointer = &(NDS_ARM7.SPSR.val);
}

FORCEINLINE void switch_arm9()
{
	dynarec_proc = 0;
	dynarec_cpu = &NDS_ARM9;
	spsr_pointer = &(NDS_ARM9.SPSR.val);
}*/


bool joker();

void update_dynarec();

void execute_arm_dynarec(u32 val);

u32 ds_read32(u32 adr);
u16 ds_read16(u32 adr);
u8 ds_read8(u32 adr);
u32 ds_write32(u32 adr, u32 val);
u32 ds_write16(u32 adr, u16 val);
u32 ds_write8(u32 adr, u8 val);

u32 read8_cycles(u32 adr);
u32 read16_cycles(u32 adr);
u32 read32_cycles(u32 adr);

u32 get_opcode();

void load_pc();
void arm_mrc(u32 pc);
void arm_mcr(u32 pc);






extern u32 reg_mode[7][7];
extern u32 cpu_modes_cpsr[7];
//extern cpu_mode_type cpu_modes[32];

extern void flush_page_table();

void dynarec_DeInit();

void dynarec_full_load();
void dynarec_full_store();

}


#endif

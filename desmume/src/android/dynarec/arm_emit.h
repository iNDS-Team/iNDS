/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
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

#ifndef ARM_EMIT_H
#define ARM_EMIT_H

#include "arm_codegen.h"
#include "dynarec_linker.h"
#include "armcpu.h"
#include "MMU.h"
#include "NDSSystem.h"

using namespace Dynarec;

extern u32 *reg;

u32 arm_update_gba_arm(u32 pc);
u32 arm_update_gba_thumb(u32 pc);
u32 arm_update_gba_idle_arm(u32 pc);
u32 arm_update_gba_idle_thumb(u32 pc);

// Although these are defined as a function, don't call them as
// such (jump to it instead)
void arm_indirect_branch_arm(u32 address);
void arm_indirect_branch_thumb(u32 address);
void arm_indirect_branch_dual_arm(u32 address);
void arm_indirect_branch_dual_thumb(u32 address);

void execute_store_cpsr(u32 new_cpsr, u32 store_mask, u32 address);
u32 execute_store_cpsr_body(u32 _cpsr, u32 store_mask, u32 address);
void execute_store_spsr(u32 new_cpsr, u32 store_mask);
u32 execute_read_spsr();
u32 execute_spsr_restore(u32 address);

void execute_swi_arm(u32 pc);
void execute_swi_thumb(u32 pc);

void execute_store_u32_safe(u32 address, u32 source);

void step_debug_arm(u32 pc);


#define write32(value)                                                        \
  *((u32 *)translation_ptr) = value;                                          \
  translation_ptr += 4                                                        \

#define arm_relative_offset(source, offset)                                   \
  (((((u32)offset - (u32)source) - 8) >> 2) & 0xFFFFFF)                       \


// reg_base_offset is the amount of bytes after reg_base where the registers
// actually begin.

#define reg_base_offset 1024


#define reg_a0          ARMREG_R0
#define reg_a1          ARMREG_R1
#define reg_a2          ARMREG_R2

#define reg_s0          ARMREG_R9
#define reg_base        ARMREG_SP
#define reg_flags       ARMREG_R11

#define reg_cycles      ARMREG_R12

#define reg_rv          ARMREG_R0

#define reg_rm          ARMREG_R0
#define reg_rn          ARMREG_R1
#define reg_rs          ARMREG_R14
#define reg_rd          ARMREG_R0


// Register allocation layout for ARM and Thumb:
// Map from a GBA register to a host ARM register. -1 means load it
// from memory into one of the temp registers.

// The following registers are chosen based on statistical analysis
// of a few games (see below), but might not be the best ones. Results
// vary tremendously between ARM and Thumb (for obvious reasons), so
// two sets are used. Take care to not call any function which can
// overwrite any of these registers from the dynarec - only call
// trusted functions in arm_stub.S which know how to save/restore
// them and know how to transfer them to the C functions it calls
// if necessary.

// The following define the actual registers available for allocation.
// As registers are freed up add them to this list.

// Note that r15 is linked to the a0 temp reg - this register will
// be preloaded with a constant upon read, and used to link to
// indirect branch functions upon write.

#define reg_x0         ARMREG_R3
#define reg_x1         ARMREG_R4
#define reg_x2         ARMREG_R5
#define reg_x3         ARMREG_R6
#define reg_x4         ARMREG_R7
#define reg_x5         ARMREG_R8

#define mem_reg        -1

/*

ARM register usage (38.775138% ARM instructions):
r00: 18.263814% (-- 18.263814%)
r12: 11.531477% (-- 29.795291%)
r09: 11.500162% (-- 41.295453%)
r14: 9.063440% (-- 50.358893%)
r06: 7.837682% (-- 58.196574%)
r01: 7.401049% (-- 65.597623%)
r07: 6.778340% (-- 72.375963%)
r05: 5.445009% (-- 77.820973%)
r02: 5.427288% (-- 83.248260%)
r03: 5.293743% (-- 88.542003%)
r04: 3.601103% (-- 92.143106%)
r11: 3.207311% (-- 95.350417%)
r10: 2.334864% (-- 97.685281%)
r08: 1.708207% (-- 99.393488%)
r15: 0.311270% (-- 99.704757%)
r13: 0.295243% (-- 100.000000%)

Thumb register usage (61.224862% Thumb instructions):
r00: 34.788858% (-- 34.788858%)
r01: 26.564083% (-- 61.352941%)
r03: 10.983500% (-- 72.336441%)
r02: 8.303127% (-- 80.639567%)
r04: 4.900381% (-- 85.539948%)
r05: 3.941292% (-- 89.481240%)
r06: 3.257582% (-- 92.738822%)
r07: 2.644851% (-- 95.383673%)
r13: 1.408824% (-- 96.792497%)
r08: 0.906433% (-- 97.698930%)
r09: 0.679693% (-- 98.378623%)
r10: 0.656446% (-- 99.035069%)
r12: 0.453668% (-- 99.488737%)
r14: 0.248909% (-- 99.737646%)
r11: 0.171066% (-- 99.908713%)
r15: 0.091287% (-- 100.000000%)

*/

s32 arm_register_allocation[] =
{
  reg_x0,       // GBA r0
  reg_x1,       // GBA r1
  mem_reg,      // GBA r2
  mem_reg,      // GBA r3
  mem_reg,      // GBA r4
  mem_reg,      // GBA r5
  reg_x2,       // GBA r6
  mem_reg,      // GBA r7
  mem_reg,      // GBA r8
  reg_x3,       // GBA r9
  mem_reg,      // GBA r10
  mem_reg,      // GBA r11
  reg_x4,       // GBA r12
  mem_reg,      // GBA r13
  reg_x5,       // GBA r14
  reg_a0        // GBA r15

  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
};

s32 thumb_register_allocation[] =
{
  reg_x0,       // GBA r0
  reg_x1,       // GBA r1
  reg_x2,       // GBA r2
  reg_x3,       // GBA r3
  reg_x4,       // GBA r4
  reg_x5,       // GBA r5
  mem_reg,      // GBA r6
  mem_reg,      // GBA r7
  mem_reg,      // GBA r8
  mem_reg,      // GBA r9
  mem_reg,      // GBA r10
  mem_reg,      // GBA r11
  mem_reg,      // GBA r12
  mem_reg,      // GBA r13
  mem_reg,      // GBA r14
  reg_a0        // GBA r15

  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
  mem_reg,
};



#define arm_imm_lsl_to_rot(value)                                             \
  (32 - value)                                                                \


__attribute__((noinline)) u32 arm_disect_imm_32bit(u32 imm, u32 *stores, u32 *rotations)
{
  u32 store_count = 0;
  u32 left_shift = 0;

  // Otherwise it'll return 0 things to store because it'll never
  // find anything.
  if(imm == 0)
  {
    rotations[0] = 0;
    stores[0] = 0;
    return 1;
  }

  // Find chunks of non-zero data at 2 bit alignments.
  while(1)
  {
    for(; left_shift < 32; left_shift += 2)
    {
      if((imm >> left_shift) & 0x03)
        break;
    }

    if(left_shift == 32)
    {
      // We've hit the end of the useful data.
      return store_count;
    }

    // Hit the end, it might wrap back around to the beginning.
    if(left_shift >= 24)
    {
      // Make a mask for the residual bits. IE, if we have
      // 5 bits of data at the end we can wrap around to 3
      // bits of data in the beginning. Thus the first
      // thing, after being shifted left, has to be less
      // than 111b, 0x7, or (1 << 3) - 1.
      u32 top_bits = 32 - left_shift;
      u32 residual_bits = 8 - top_bits;
      u32 residual_mask = (1 << residual_bits) - 1;

      if((store_count > 1) && (left_shift > 24) &&
       ((stores[0] << ((32 - rotations[0]) & 0x1F)) < residual_mask))
      {
        // Then we can throw out the last bit and tack it on
        // to the first bit.
        stores[0] =
         (stores[0] << ((top_bits + (32 - rotations[0])) & 0x1F)) |
         ((imm >> left_shift) & 0xFF);
        rotations[0] = top_bits;

        return store_count;
      }
      else
      {
        // There's nothing to wrap over to in the beginning
        stores[store_count] = (imm >> left_shift) & 0xFF;
        rotations[store_count] = (32 - left_shift) & 0x1F;
        return store_count + 1;
      }
      break;
    }

    stores[store_count] = (imm >> left_shift) & 0xFF;
    rotations[store_count] = (32 - left_shift) & 0x1F;

    store_count++;
    left_shift += 8;
  }

  return 0;
}

#define arm_load_imm_32bit(ireg, imm)                                         \
{                                                                             \
  u32 stores[4];                                                              \
  u32 rotations[4];                                                           \
  u32 store_count = arm_disect_imm_32bit(imm, stores, rotations);             \
  u32 i;                                                                      \
                                                                              \
  ARM_MOV_REG_IMM(0, ireg, stores[0], rotations[0]);                          \
                                                                              \
  for(i = 1; i < store_count; i++)                                            \
  {                                                                           \
    ARM_ORR_REG_IMM(0, ireg, ireg, stores[i], rotations[i]);                  \
  }                                                                           \
}                                                                             \


#define generate_load_pc(ireg, new_pc)                                        \
  arm_load_imm_32bit(ireg, new_pc)                                            \

#define generate_load_imm(ireg, imm, imm_ror)                                 \
  ARM_MOV_REG_IMM(0, ireg, imm, imm_ror)                                      \



#define generate_shift_left(ireg, imm)                                        \
  ARM_MOV_REG_IMMSHIFT(0, ireg, ireg, ARMSHIFT_LSL, imm)                      \

#define generate_shift_right(ireg, imm)                                       \
  ARM_MOV_REG_IMMSHIFT(0, ireg, ireg, ARMSHIFT_LSR, imm)                      \

#define generate_shift_right_arithmetic(ireg, imm)                            \
  ARM_MOV_REG_IMMSHIFT(0, ireg, ireg, ARMSHIFT_ASR, imm)                      \

#define generate_rotate_right(ireg, imm)                                      \
  ARM_MOV_REG_IMMSHIFT(0, ireg, ireg, ARMSHIFT_ROR, imm)                      \

#define generate_add(ireg_dest, ireg_src)                                     \
  ARM_ADD_REG_REG(0, ireg_dest, ireg_dest, ireg_src)                          \

#define generate_sub(ireg_dest, ireg_src)                                     \
  ARM_SUB_REG_REG(0, ireg_dest, ireg_dest, ireg_src)                          \

#define generate_or(ireg_dest, ireg_src)                                      \
  ARM_ORR_REG_REG(0, ireg_dest, ireg_dest, ireg_src)                          \

#define generate_xor(ireg_dest, ireg_src)                                     \
  ARM_EOR_REG_REG(0, ireg_dest, ireg_dest, ireg_src)                          \

#define generate_add_imm(ireg, imm, imm_ror)                                  \
  ARM_ADD_REG_IMM(0, ireg, ireg, imm, imm_ror)                                \

#define generate_sub_imm(ireg, imm, imm_ror)                                  \
  ARM_SUB_REG_IMM(0, ireg, ireg, imm, imm_ror)                                \

#define generate_xor_imm(ireg, imm, imm_ror)                                  \
  ARM_EOR_REG_IMM(0, ireg, ireg, imm, imm_ror)                                \

#define generate_add_reg_reg_imm(ireg_dest, ireg_src, imm, imm_ror)           \
  ARM_ADD_REG_IMM(0, ireg_dest, ireg_src, imm, imm_ror)                       \

#define generate_and_imm(ireg, imm, imm_ror)                                  \
  ARM_AND_REG_IMM(0, ireg, ireg, imm, imm_ror)                                \

#define generate_mov(ireg_dest, ireg_src)                                     \
  if(ireg_dest != ireg_src)                                                   \
  {                                                                           \
    ARM_MOV_REG_REG(0, ireg_dest, ireg_src);                                  \
  }                                                                           \

//#define generate_function_call(function_location)                             \
//  do                            											 \
//  {                           												  \
//  ARM_BL(0, arm_relative_offset(translation_ptr, function_location));          \
//  LOGE("gen fun call : %x %x %x", (u32)translation_ptr,(u32)function_location,((u32)function_location-(u32)translation_ptr - 8)>>2);\
//  }while(0);

#define generate_function_call(function_location)                             \
  do                            											 \
  {                           												  \
	  arm_load_imm_32bit(ARMREG_LR,(u32)function_location);					  \
	  ARM_BLX(0, ARMREG_LR);												 \
  }while(0);

#define generate_function_call_step1(function_location)                         \
  do                            											 \
  {                           												  \
	  arm_load_imm_32bit(ARMREG_LR,(u32)function_location);					  \
  }while(0);

#define generate_function_call_step2(function_location)                         \
do                            											 \
{                           												  \
	 ARM_BLX(0, ARMREG_LR);												 \
}while(0);

#define generate_exit_block()                                                 \
  ARM_BX(0, ARMREG_LR)                                                        \

// The branch target is to be filled in later (thus a 0 for now)

#define generate_branch_filler(condition_code, writeback_location)            \
  (writeback_location) = translation_ptr;                                     \
  ARM_B_COND(0, condition_code, 0)                                            \

#define generate_update_pc(new_pc)                                            \
  generate_load_pc(reg_a0, new_pc)                                            \

#define generate_cycle_update()                                               \
  if(cycle_count)                                                             \
  {                                                                           \
    if(cycle_count >> 8)                                                      \
    {                                                                         \
      ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, (cycle_count >> 8) & 0xFF,   \
       arm_imm_lsl_to_rot(8));                                                \
    }                                                                         \
    ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, (cycle_count & 0xFF), 0);      \
    cycle_count = 0;                                                          \
  }                                                                           \

#define generate_cycle_update_flag_set()                                      \
  if(cycle_count >> 8)                                                        \
  {                                                                           \
    ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, (cycle_count >> 8) & 0xFF,     \
     arm_imm_lsl_to_rot(8));                                                  \
  }                                                                           \
  generate_save_flags();                                                      \
  ARM_ADDS_REG_IMM(0, reg_cycles, reg_cycles, (cycle_count & 0xFF), 0);       \
  cycle_count = 0                                                             \

#define generate_branch_patch_conditional_arm(dest, offset)                       \
	generate_cycle_update();\
  ARM_SUB_REG_IMM(0, reg_cycles, reg_cycles, (((pc - cond_pc)>>2) & 0xFF), 0)  ;\
  *((u32 *)(dest)) = (*((u32 *)dest) & 0xFF000000) |                          \
   arm_relative_offset(dest, offset);                                          \
   ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, (((pc - cond_pc)>>2) & 0xFF), 0)  \


#define generate_branch_patch_conditional_thumb(dest, offset)                       \
		ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, 2, 0);  \
  *((u32 *)(dest)) = (*((u32 *)dest) & 0xFF000000) |                          \
   arm_relative_offset(dest, offset);                                          \
   ARM_ADD_REG_IMM(0, reg_cycles, reg_cycles, 1, 0)  \

#define generate_branch_patch_unconditional(dest, offset)                     \
  *((u32 *)(dest)) = (*((u32 *)dest) & 0xFF000000) |                          \
   arm_relative_offset(dest, offset)                                          \


// A different function is called for idle updates because of the relative
// location of the embedded PC. The idle version could be optimized to put
// the CPU into halt mode too, however.

#define generate_branch_idle_eliminate(writeback_location, new_pc, mode)      \
  generate_function_call(arm_update_gba_idle_##mode);                         \
  write32(new_pc);                                                            \
  generate_branch_filler(ARMCOND_AL, writeback_location)                      \

#define generate_branch_update(writeback_location, new_pc, mode)              \
  generate_function_call_step1(arm_update_gba_##mode);                        \
  ARM_MOV_REG_IMMSHIFT(0, reg_a0, reg_cycles, ARMSHIFT_LSR, 31);              \
  ARM_ADD_REG_IMMSHIFT(0, ARMREG_PC, ARMREG_PC, reg_a0, ARMSHIFT_LSL, 2);     \
  write32(new_pc);                                                            \
  generate_function_call_step2(arm_update_gba_##mode);                        \
  generate_branch_filler(ARMCOND_AL, writeback_location)                      \


#define generate_branch_no_cycle_update(writeback_location, new_pc, mode)     \
  if(pc == idle_loop_target_pc)                                               \
  {                                                                           \
    generate_branch_idle_eliminate(writeback_location, new_pc, mode);         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_branch_update(writeback_location, new_pc, mode);                 \
  }                                                                           \

#define generate_branch_cycle_update(writeback_location, new_pc, mode)        \
  generate_cycle_update();                                                    \
  generate_branch_no_cycle_update(writeback_location, new_pc, mode)           \

// a0 holds the destination

//#define generate_indirect_branch_no_cycle_update(type)                        \
//  ARM_B(0, arm_relative_offset(translation_ptr, arm_indirect_branch_##type))  \

//#define generate_indirect_branch_no_cycle_update(type)                        \
//  do                            											 \
//  {                           												  \
//	  ARM_STR_IMM(0,ARMREG_R0,reg_base,(reg_base_offset + (62 * 4)));	      \
//	  arm_load_imm_32bit(ARMREG_R0,(u32)(arm_indirect_branch_##type));		 \
//	  ARM_STR_IMM(0,ARMREG_R0,reg_base,(reg_base_offset + (63 * 4)));	      \
//	  ARM_LDR_IMM(0,ARMREG_R0,reg_base,(reg_base_offset + (62 * 4)));	      \
//	  ARM_LDR_IMM(0,ARMREG_PC,reg_base,(reg_base_offset + (63 * 4)));	      \
//  }while(0);

#define generate_indirect_branch_no_cycle_update(type)                        \
  do                            											 \
  {                           												  \
	  arm_load_imm_32bit(ARMREG_R1,(u32)(arm_indirect_branch_##type));		 \
	  ARM_BX(0, ARMREG_R1);  												\
  }while(0);

#define generate_indirect_branch_cycle_update(type)                           \
  generate_cycle_update();                                                    \
  generate_indirect_branch_no_cycle_update(type)                              \

#define generate_block_prologue()                                             \

#define generate_block_extra_vars_arm()                                       \
  void generate_indirect_branch_arm()                                         \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
    generate_indirect_branch_no_cycle_update(arm);                            \
  }                                                                           \
                                                                              \
  void generate_indirect_branch_dual()                                        \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
    generate_indirect_branch_no_cycle_update(dual_arm);                       \
  }                                                                           \
                                                                              \
  u32 prepare_load_reg(u32 scratch_reg, u32 reg_index)                        \
  {                                                                           \
    u32 reg_use = arm_register_allocation[reg_index];                         \
    if(reg_use == mem_reg)                                                    \
    {                                                                         \
      ARM_LDR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
      return scratch_reg;                                                     \
    }                                                                         \
                                                                              \
    return reg_use;                                                           \
  }                                                                           \
                                                                              \
  u32 prepare_load_reg_pc(u32 scratch_reg, u32 reg_index, u32 pc_offset)      \
  {                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_load_pc(scratch_reg, pc + pc_offset);                          \
      return scratch_reg;                                                     \
    }                                                                         \
    return prepare_load_reg(scratch_reg, reg_index);                          \
  }                                                                           \
                                                                              \
  u32 prepare_store_reg(u32 scratch_reg, u32 reg_index)                       \
  {                                                                           \
    u32 reg_use = arm_register_allocation[reg_index];                         \
    if(reg_use == mem_reg)                                                    \
      return scratch_reg;                                                     \
                                                                              \
    return reg_use;                                                           \
  }                                                                           \
                                                                              \
  void complete_store_reg(u32 scratch_reg, u32 reg_index)                     \
  {                                                                           \
    if(arm_register_allocation[reg_index] == mem_reg)                         \
    {                                                                         \
      ARM_STR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  void complete_store_reg_pc_no_flags(u32 scratch_reg, u32 reg_index)         \
  {                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_indirect_branch_arm();                                         \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      complete_store_reg(scratch_reg, reg_index);                             \
    }                                                                         \
  }                                                                           \
                                                                              \
  void complete_store_reg_pc_flags(u32 scratch_reg, u32 reg_index)            \
  {                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      if(condition == 0x0E)                                                   \
      {                                                                       \
        generate_cycle_update();                                              \
      }                                                                       \
      generate_function_call(execute_spsr_restore);                           \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      complete_store_reg(scratch_reg, reg_index);                             \
    }                                                                         \
  }                                                                           \
                                                                              \
  void generate_load_reg(u32 ireg, u32 reg_index)                             \
  {                                                                           \
    s32 load_src = arm_register_allocation[reg_index];                        \
    if(load_src != mem_reg)                                                   \
    {                                                                         \
      ARM_MOV_REG_REG(0, ireg, load_src);                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_LDR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  }                                                                           \
                                                                              \
  void generate_store_reg(u32 ireg, u32 reg_index)                            \
  {                                                                           \
    s32 store_dest = arm_register_allocation[reg_index];                      \
    if(store_dest != mem_reg)                                                 \
    {                                                                         \
      ARM_MOV_REG_REG(0, store_dest, ireg);                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_STR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  }                                                                           \


#define generate_block_extra_vars_thumb()                                     \
  u32 prepare_load_reg(u32 scratch_reg, u32 reg_index)                        \
  {                                                                           \
    u32 reg_use = thumb_register_allocation[reg_index];                       \
    if(reg_use == mem_reg)                                                    \
    {                                                                         \
      ARM_LDR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
      return scratch_reg;                                                     \
    }                                                                         \
                                                                              \
    return reg_use;                                                           \
  }                                                                           \
                                                                              \
  u32 prepare_load_reg_pc(u32 scratch_reg, u32 reg_index, u32 pc_offset)      \
  {                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_load_pc(scratch_reg, pc + pc_offset);                          \
      return scratch_reg;                                                     \
    }                                                                         \
    return prepare_load_reg(scratch_reg, reg_index);                          \
  }                                                                           \
                                                                              \
  u32 prepare_store_reg(u32 scratch_reg, u32 reg_index)                       \
  {                                                                           \
    u32 reg_use = thumb_register_allocation[reg_index];                       \
    if(reg_use == mem_reg)                                                    \
      return scratch_reg;                                                     \
                                                                              \
    return reg_use;                                                           \
  }                                                                           \
                                                                              \
  void complete_store_reg(u32 scratch_reg, u32 reg_index)                     \
  {                                                                           \
    if(thumb_register_allocation[reg_index] == mem_reg)                       \
    {                                                                         \
      ARM_STR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  void generate_load_reg(u32 ireg, u32 reg_index)                             \
  {                                                                           \
    s32 load_src = thumb_register_allocation[reg_index];                      \
    if(load_src != mem_reg)                                                   \
    {                                                                         \
      ARM_MOV_REG_REG(0, ireg, load_src);                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_LDR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  }                                                                           \
                                                                              \
  void generate_store_reg(u32 ireg, u32 reg_index)                            \
  {                                                                           \
    s32 store_dest = thumb_register_allocation[reg_index];                    \
    if(store_dest != mem_reg)                                                 \
    {                                                                         \
      ARM_MOV_REG_REG(0, store_dest, ireg);                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_STR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  }                                                                           \

u8 *last_rom_translation_ptr = rom_translation_cache;
//u8 *last_ram_translation_ptr = ram_translation_cache;
u8 *last_bios_translation_ptr = bios_translation_cache;

/*
#define translate_invalidate_dcache_one(which)                                \
  if (which##_translation_ptr > last_##which##_translation_ptr)               \
  {                                                                           \
    warm_cache_op_range(WOP_D_CLEAN, last_##which##_translation_ptr,          \
      which##_translation_ptr - last_##which##_translation_ptr);              \
    warm_cache_op_range(WOP_I_INVALIDATE, last_##which##_translation_ptr, 32);\
    last_##which##_translation_ptr = which##_translation_ptr;                 \
  }
*/

#define translate_invalidate_dcache_one(which)                                \
  if (which##_translation_ptr > last_##which##_translation_ptr)               \
  {                                                                           \
	__builtin___clear_cache(last_##which##_translation_ptr, which##_translation_ptr);              \
    last_##which##_translation_ptr = which##_translation_ptr;                 \
  }

/*  
#define translate_invalidate_dcache(dorom,doram)                                         \
{                                                                             \
  translate_invalidate_dcache_one(rom)                                        \
  translate_invalidate_dcache_one(ram)                                        \
  translate_invalidate_dcache_one(bios)                                       \
}
*/

#define translate_invalidate_dcache()     	                                  \
{                                                                             \
  translate_invalidate_dcache_one(rom)                              		  \
  translate_invalidate_dcache_one(bios)                      	  			  \
}

#define invalidate_icache_region(addr, size)                                  \
  warm_cache_op_range(WOP_I_INVALIDATE, addr, size)


#define block_prologue_size 0


// It should be okay to still generate result flags, spsr will overwrite them.
// This is pretty infrequent (returning from interrupt handlers, et al) so
// probably not worth optimizing for.

#define check_for_interrupts()                                                \
if(exec_interrupts(ARMCPU_ARM9))\
{\
    reg_mode[MODE_IRQ][6] = pc + 4;                                           \
    spsr[MODE_IRQ] = reg[REG_CPSR];                                           \
    reg[REG_CPSR] = 0xD2;                                                     \
    pc = cpu.intVector + 0x18;                                                          \
    set_cpu_mode(MODE_IRQ);                                                   \
    armcpu_irqException(&cpu);\
}\

#define generate_load_reg_pc(ireg, reg_index, pc_offset)                      \
  if(reg_index == 15)                                                         \
  {                                                                           \
    generate_load_pc(ireg, pc + pc_offset);                                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_reg(ireg, reg_index);                                       \
  }                                                                           \

#define generate_store_reg_pc_no_flags(ireg, reg_index)                       \
  generate_store_reg(ireg, reg_index);                                        \
  if(reg_index == 15)                                                         \
  {                                                                           \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

u32 function_cc execute_spsr_restore_body(u32 pc)
{
  armcpu_switchMode(dynarec_cpu, dynarec_cpu->SPSR.bits.mode);
  dynarec_cpu->changeCPSR();

  //execHardware_interrupts();
//  check_for_interrupts();
  return pc;
}


#define generate_store_reg_pc_flags(ireg, reg_index)                          \
  generate_store_reg(ireg, reg_index);                                        \
  if(reg_index == 15)                                                         \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
    generate_function_call(execute_spsr_restore);                             \
  }                                                                           \


#define generate_load_flags()                                                 \
/*  ARM_MSR_REG(0, ARM_PSR_F, reg_flags, ARM_CPSR) */                         \

#define generate_store_flags()                                                \
/*  ARM_MRS_CPSR(0, reg_flags) */                                             \

#define generate_save_flags()                                                 \
  ARM_MRS_CPSR(0, reg_flags)                                                  \

#define generate_restore_flags()                                              \
  ARM_MSR_REG(0, ARM_PSR_F, reg_flags, ARM_CPSR)                              \


#define condition_opposite_eq ARMCOND_NE
#define condition_opposite_ne ARMCOND_EQ
#define condition_opposite_cs ARMCOND_CC
#define condition_opposite_cc ARMCOND_CS
#define condition_opposite_mi ARMCOND_PL
#define condition_opposite_pl ARMCOND_MI
#define condition_opposite_vs ARMCOND_VC
#define condition_opposite_vc ARMCOND_VS
#define condition_opposite_hi ARMCOND_LS
#define condition_opposite_ls ARMCOND_HI
#define condition_opposite_ge ARMCOND_LT
#define condition_opposite_lt ARMCOND_GE
#define condition_opposite_gt ARMCOND_LE
#define condition_opposite_le ARMCOND_GT
#define condition_opposite_al ARMCOND_NV
#define condition_opposite_nv ARMCOND_AL

#define generate_branch(mode)                                                 \
{                                                                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target, mode);                     \
  block_exit_position++;                                                      \
}                                                                             \

#define generate_branch_blx_imm(mode)                                                 \
{                                                                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target & 0xFFFFFFFE, mode);                     \
  block_exit_position++;                                                      \
}                                                                             \


#define generate_op_and_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_AND_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_orr_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_ORR_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_eor_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_EOR_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_bic_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_BIC_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_sub_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_SUB_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_rsb_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_RSB_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_sbc_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_SBC_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_rsc_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_RSC_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_add_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_ADD_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_adc_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_ADC_REG_IMMSHIFT(0, _rd, _rn, _rm, shift_type, shift)                   \

#define generate_op_mov_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_MOV_REG_IMMSHIFT(0, _rd, _rm, shift_type, shift)                        \

#define generate_op_mvn_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  ARM_MVN_REG_IMMSHIFT(0, _rd, _rm, shift_type, shift)                        \


#define generate_op_and_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_AND_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_orr_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_ORR_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_eor_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_EOR_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_bic_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_BIC_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_sub_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_SUB_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_rsb_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_RSB_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_sbc_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_SBC_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_rsc_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_RSC_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_add_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_ADD_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_adc_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_ADC_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                     \

#define generate_op_mov_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_MOV_REG_REGSHIFT(0, _rd, _rm, shift_type, _rs)                          \

#define generate_op_mvn_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  ARM_MVN_REG_REGSHIFT(0, _rd, _rm, shift_type, _rs)                          \


#define generate_op_and_imm(_rd, _rn)                                         \
  ARM_AND_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_orr_imm(_rd, _rn)                                         \
  ARM_ORR_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_eor_imm(_rd, _rn)                                         \
  ARM_EOR_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_bic_imm(_rd, _rn)                                         \
  ARM_BIC_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_sub_imm(_rd, _rn)                                         \
  ARM_SUB_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_rsb_imm(_rd, _rn)                                         \
  ARM_RSB_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_sbc_imm(_rd, _rn)                                         \
  ARM_SBC_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_rsc_imm(_rd, _rn)                                         \
  ARM_RSC_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_add_imm(_rd, _rn)                                         \
  ARM_ADD_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_adc_imm(_rd, _rn)                                         \
  ARM_ADC_REG_IMM(0, _rd, _rn, imm, imm_ror)                                  \

#define generate_op_mov_imm(_rd, _rn)                                         \
  ARM_MOV_REG_IMM(0, _rd, imm, imm_ror)                                       \

#define generate_op_mvn_imm(_rd, _rn)                                         \
  ARM_MVN_REG_IMM(0, _rd, imm, imm_ror)                                       \


#define generate_op_reg_immshift_lflags(name, _rd, _rn, _rm, st, shift)       \
  ARM_##name##_REG_IMMSHIFT(0, _rd, _rn, _rm, st, shift)                      \

#define generate_op_reg_immshift_aflags(name, _rd, _rn, _rm, st, shift)       \
  ARM_##name##_REG_IMMSHIFT(0, _rd, _rn, _rm, st, shift)                      \

#define generate_op_reg_immshift_aflags_load_c(name, _rd, _rn, _rm, st, sh)   \
  ARM_##name##_REG_IMMSHIFT(0, _rd, _rn, _rm, st, sh)                         \

#define generate_op_reg_immshift_uflags(name, _rd, _rm, shift_type, shift)    \
  ARM_##name##_REG_IMMSHIFT(0, _rd, _rm, shift_type, shift)                   \

#define generate_op_reg_immshift_tflags(name, _rn, _rm, shift_type, shift)    \
  ARM_##name##_REG_IMMSHIFT(0, _rn, _rm, shift_type, shift)                   \


#define generate_op_reg_regshift_lflags(name, _rd, _rn, _rm, shift_type, _rs) \
  ARM_##name##_REG_REGSHIFT(0, _rd, _rn, _rm, shift_type, _rs)                \

#define generate_op_reg_regshift_aflags(name, _rd, _rn, _rm, st, _rs)         \
  ARM_##name##_REG_REGSHIFT(0, _rd, _rn, _rm, st, _rs)                        \

#define generate_op_reg_regshift_aflags_load_c(name, _rd, _rn, _rm, st, _rs)  \
  ARM_##name##_REG_REGSHIFT(0, _rd, _rn, _rm, st, _rs)                        \

#define generate_op_reg_regshift_uflags(name, _rd, _rm, shift_type, _rs)      \
  ARM_##name##_REG_REGSHIFT(0, _rd, _rm, shift_type, _rs)                     \

#define generate_op_reg_regshift_tflags(name, _rn, _rm, shift_type, _rs)      \
  ARM_##name##_REG_REGSHIFT(0, _rn, _rm, shift_type, _rs)                     \


#define generate_op_imm_lflags(name, _rd, _rn)                                \
  ARM_##name##_REG_IMM(0, _rd, _rn, imm, imm_ror)                             \

#define generate_op_imm_aflags(name, _rd, _rn)                                \
  ARM_##name##_REG_IMM(0, _rd, _rn, imm, imm_ror)                             \

#define generate_op_imm_aflags_load_c(name, _rd, _rn)                         \
  ARM_##name##_REG_IMM(0, _rd, _rn, imm, imm_ror)                             \

#define generate_op_imm_uflags(name, _rd)                                     \
  ARM_##name##_REG_IMM(0, _rd, imm, imm_ror)                                  \

#define generate_op_imm_tflags(name, _rn)                                     \
  ARM_##name##_REG_IMM(0, _rn, imm, imm_ror)                                  \


#define generate_op_ands_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_lflags(ANDS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_orrs_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_lflags(ORRS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_eors_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_lflags(EORS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_bics_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_lflags(BICS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_subs_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_aflags(SUBS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_rsbs_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_aflags(RSBS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_sbcs_reg_immshift(_rd, _rn, _rm, st, shift)               \
  generate_op_reg_immshift_aflags_load_c(SBCS, _rd, _rn, _rm, st, shift)      \

#define generate_op_rscs_reg_immshift(_rd, _rn, _rm, st, shift)               \
  generate_op_reg_immshift_aflags_load_c(RSCS, _rd, _rn, _rm, st, shift)      \

#define generate_op_adds_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_aflags(ADDS, _rd, _rn, _rm, shift_type, shift)     \

#define generate_op_adcs_reg_immshift(_rd, _rn, _rm, st, shift)               \
  generate_op_reg_immshift_aflags_load_c(ADCS, _rd, _rn, _rm, st, shift)      \

#define generate_op_movs_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_uflags(MOVS, _rd, _rm, shift_type, shift)          \

#define generate_op_mvns_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_op_reg_immshift_uflags(MVNS, _rd, _rm, shift_type, shift)          \

// The reg operand is in reg_rm, not reg_rn like expected, so rsbs isn't
// being used here. When rsbs is fully inlined it can be used with the
// apropriate operands.

#define generate_op_neg_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
{                                                                             \
  generate_load_imm(reg_rn, 0, 0);                                            \
  generate_op_subs_reg_immshift(_rd, reg_rn, _rm, ARMSHIFT_LSL, 0);           \
}                                                                             \

#define generate_op_muls_reg_immshift(_rd, _rn, _rm, shift_type, shift)       \
  generate_load_flags();                                                      \
  ARM_MULS(0, _rd, _rn, _rm);                                                 \
  generate_store_flags()                                                      \

#define generate_op_cmp_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  generate_op_reg_immshift_tflags(CMP, _rn, _rm, shift_type, shift)           \

#define generate_op_cmn_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  generate_op_reg_immshift_tflags(CMN, _rn, _rm, shift_type, shift)           \

#define generate_op_tst_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  generate_op_reg_immshift_tflags(TST, _rn, _rm, shift_type, shift)           \

#define generate_op_teq_reg_immshift(_rd, _rn, _rm, shift_type, shift)        \
  generate_op_reg_immshift_tflags(TEQ, _rn, _rm, shift_type, shift)           \


#define generate_op_ands_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_lflags(ANDS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_orrs_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_lflags(ORRS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_eors_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_lflags(EORS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_bics_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_lflags(BICS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_subs_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_aflags(SUBS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_rsbs_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_aflags(RSBS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_sbcs_reg_regshift(_rd, _rn, _rm, st, _rs)                 \
  generate_op_reg_regshift_aflags_load_c(SBCS, _rd, _rn, _rm, st, _rs)        \

#define generate_op_rscs_reg_regshift(_rd, _rn, _rm, st, _rs)                 \
  generate_op_reg_regshift_aflags_load_c(RSCS, _rd, _rn, _rm, st, _rs)        \

#define generate_op_adds_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_aflags(ADDS, _rd, _rn, _rm, shift_type, _rs)       \

#define generate_op_adcs_reg_regshift(_rd, _rn, _rm, st, _rs)                 \
  generate_op_reg_regshift_aflags_load_c(ADCS, _rd, _rn, _rm, st, _rs)        \

#define generate_op_movs_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_uflags(MOVS, _rd, _rm, shift_type, _rs)            \

#define generate_op_mvns_reg_regshift(_rd, _rn, _rm, shift_type, _rs)         \
  generate_op_reg_regshift_uflags(MVNS, _rd, _rm, shift_type, _rs)            \

#define generate_op_cmp_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  generate_op_reg_regshift_tflags(CMP, _rn, _rm, shift_type, _rs)             \

#define generate_op_cmn_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  generate_op_reg_regshift_tflags(CMN, _rn, _rm, shift_type, _rs)             \

#define generate_op_tst_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  generate_op_reg_regshift_tflags(TST, _rn, _rm, shift_type, _rs)             \

#define generate_op_teq_reg_regshift(_rd, _rn, _rm, shift_type, _rs)          \
  generate_op_reg_regshift_tflags(TEQ, _rn, _rm, shift_type, _rs)             \


#define generate_op_ands_imm(_rd, _rn)                                        \
  generate_op_imm_lflags(ANDS, _rd, _rn)                                      \

#define generate_op_orrs_imm(_rd, _rn)                                        \
  generate_op_imm_lflags(ORRS, _rd, _rn)                                      \

#define generate_op_eors_imm(_rd, _rn)                                        \
  generate_op_imm_lflags(EORS, _rd, _rn)                                      \

#define generate_op_bics_imm(_rd, _rn)                                        \
  generate_op_imm_lflags(BICS, _rd, _rn)                                      \

#define generate_op_subs_imm(_rd, _rn)                                        \
  generate_op_imm_aflags(SUBS, _rd, _rn)                                      \

#define generate_op_rsbs_imm(_rd, _rn)                                        \
  generate_op_imm_aflags(RSBS, _rd, _rn)                                      \

#define generate_op_sbcs_imm(_rd, _rn)                                        \
  generate_op_imm_aflags_load_c(SBCS, _rd, _rn)                               \

#define generate_op_rscs_imm(_rd, _rn)                                        \
  generate_op_imm_aflags_load_c(RSCS, _rd, _rn)                               \

#define generate_op_adds_imm(_rd, _rn)                                        \
  generate_op_imm_aflags(ADDS, _rd, _rn)                                      \

#define generate_op_adcs_imm(_rd, _rn)                                        \
  generate_op_imm_aflags_load_c(ADCS, _rd, _rn)                               \

#define generate_op_movs_imm(_rd, _rn)                                        \
  generate_op_imm_uflags(MOVS, _rd)                                           \

#define generate_op_mvns_imm(_rd, _rn)                                        \
  generate_op_imm_uflags(MVNS, _rd)                                           \

#define generate_op_cmp_imm(_rd, _rn)                                         \
  generate_op_imm_tflags(CMP, _rn)                                            \

#define generate_op_cmn_imm(_rd, _rn)                                         \
  generate_op_imm_tflags(CMN, _rn)                                            \

#define generate_op_tst_imm(_rd, _rn)                                         \
  generate_op_imm_tflags(TST, _rn)                                            \

#define generate_op_teq_imm(_rd, _rn)                                         \
  generate_op_imm_tflags(TEQ, _rn)                                            \


#define prepare_load_rn_yes()                                                 \
  u32 _rn = prepare_load_reg_pc(reg_rn, rn, 8)                                \

#define prepare_load_rn_no()                                                  \

#define prepare_store_rd_yes()                                                \
  u32 _rd = prepare_store_reg(reg_rd, rd)                                     \

#define prepare_store_rd_no()                                                 \

#define complete_store_rd_yes(flags_op)                                       \
  complete_store_reg_pc_##flags_op(_rd, rd)                                   \

#define complete_store_rd_no(flags_op)                                        \

#define arm_generate_op_reg(name, load_op, store_op, flags_op)                \
  u32 shift_type = (opcode >> 5) & 0x03;                                      \
  arm_decode_data_proc_reg();                                                 \
  prepare_load_rn_##load_op();                                                \
  prepare_store_rd_##store_op();                                              \
                                                                              \
  if((opcode >> 4) & 0x01)                                                    \
  {                                                                           \
	exec_cyc=( rd == 15 ? 4 : 2);                                               \
    u32 rs = ((opcode >> 8) & 0x0F);                                          \
    u32 _rs = prepare_load_reg(reg_rs, rs);                                   \
    u32 _rm = prepare_load_reg_pc(reg_rm, rm, 12);                            \
    generate_op_##name##_reg_regshift(_rd, _rn, _rm, shift_type, _rs);        \
  }                                                                           \
  else                                                                        \
  {                                                                           \
	exec_cyc=( rd == 15 ? 3 : 1);                                               \
    u32 shift_imm = ((opcode >> 7) & 0x1F);                                   \
    u32 _rm = prepare_load_reg_pc(reg_rm, rm, 8);                             \
    generate_op_##name##_reg_immshift(_rd, _rn, _rm, shift_type, shift_imm);  \
  }                                                                           \
  complete_store_rd_##store_op(flags_op)                                      \

#define arm_generate_op_reg_flags(name, load_op, store_op, flags_op)          \
  arm_generate_op_reg(name, load_op, store_op, flags_op)                      \

// imm will be loaded by the called function if necessary.

#define arm_generate_op_imm(name, load_op, store_op, flags_op)                \
  arm_decode_data_proc_imm();                                                 \
  exec_cyc=( rd == 15 ? 3 : 1);                                               \
  prepare_load_rn_##load_op();                                                \
  prepare_store_rd_##store_op();                                              \
  generate_op_##name##_imm(_rd, _rn);                                         \
  complete_store_rd_##store_op(flags_op)                                      \

#define arm_generate_op_imm_flags(name, load_op, store_op, flags_op)          \
  arm_generate_op_imm(name, load_op, store_op, flags_op)                      \

#define arm_data_proc(name, type, flags_op)                                   \
{                                                                             \
  arm_generate_op_##type(name, yes, yes, flags_op);                           \
}                                                                             \

#define reg_cycle_val 2
#define imm_cycle_val 1
#define reg_flags_cycle_val 2
#define imm_flags_cycle_val 1

#define arm_data_proc_test(name, type)                                        \
{                                                                             \
  arm_generate_op_##type(name, yes, no, no);                                  \
}                                                                             \

#define arm_data_proc_unary(name, type, flags_op)                             \
{                                                                             \
  arm_generate_op_##type(name, no, yes, flags_op);                            \
}                                                                             \


#define arm_multiply_add_no_flags_no()                                        \
  ARM_MUL(0, _rd, _rm, _rs)                                                   \

#define arm_multiply_add_yes_flags_no()                                       \
  u32 _rn = prepare_load_reg(reg_a2, rn);                                     \
  ARM_MLA(0, _rd, _rm, _rs, _rn)                                              \

#define arm_multiply_add_no_flags_yes()                                       \
  generate_load_flags();                                                      \
  ARM_MULS(0, _rd, _rm, _rs)                                         \
  generate_store_flags()                                                      \

#define arm_multiply_add_yes_flags_yes()                                      \
  u32 _rn = prepare_load_reg(reg_a2, rn);                                     \
  generate_load_flags();                                                      \
  ARM_MLAS(0, _rd, _rm, _rs, _rn);                                            \
  generate_store_flags()

extern void multiply_add_no_cycles();
extern void multiply_add_yes_cycles();

#define arm_multiply(add_op, flags)                                           \
{                                                                             \
  arm_decode_multiply();                                                      \
  u32 _rm = prepare_load_reg(reg_a0, rm);                                     \
  ARM_MOV_REG_REG(0, reg_a0, _rm);\
  generate_function_call(multiply_add_##add_op##_cycles);\
  u32 _rs = prepare_load_reg(reg_a1, rs);                                     \
  u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
  arm_multiply_add_##add_op##_flags_##flags();                                \
  complete_store_reg(_rd, rd);                                                \
}                                                                             \

extern void u64_cycles();
extern void u64_add_cycles();
extern void s64_cycles();
extern void s64_add_cycles();

#define arm_multiply_long_name_s64     SMULL
#define arm_multiply_long_name_u64     UMULL
#define arm_multiply_long_name_s64_add SMLAL
#define arm_multiply_long_name_u64_add UMLAL


#define arm_multiply_long_flags_no(name)                                      \
  ARM_##name(0, _rdlo, _rdhi, _rm, _rs)                                       \

#define arm_multiply_long_flags_yes(name)                                     \
  generate_load_flags();                                                      \
  ARM_##name##S(0, _rdlo, _rdhi, _rm, _rs);                                   \
  generate_store_flags()                                                      \


#define arm_multiply_long_add_no(name)                                        \

#define arm_multiply_long_add_yes(name)                                       \
  prepare_load_reg(reg_a0, rdlo);                                             \
  prepare_load_reg(reg_a1, rdhi)                                              \


#define arm_multiply_long_op(flags, name)                                     \
  arm_multiply_long_flags_##flags(name)                                       \

#define arm_multiply_long(name, add_op, flags)                                \
{                                                                             \
  arm_decode_multiply_long();                                                 \
  u32 _rm = prepare_load_reg(reg_a2, rm);                                     \
  ARM_MOV_REG_REG(0, reg_a2, _rm); \
  generate_function_call(name##_cycles);\
  u32 _rs = prepare_load_reg(reg_rs, rs);                                     \
  u32 _rdlo = prepare_store_reg(reg_a0, rdlo);                                \
  u32 _rdhi = prepare_store_reg(reg_a1, rdhi);                                \
  arm_multiply_long_add_##add_op(name);                                       \
  arm_multiply_long_op(flags, arm_multiply_long_name_##name);                 \
  complete_store_reg(_rdlo, rdlo);                                            \
  complete_store_reg(_rdhi, rdhi);                                            \
}                                                                             \

#define arm_psr_read_cpsr()                                                   \
  u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
  generate_load_reg(_rd, REG_CPSR);                                           \
  ARM_BIC_REG_IMM(0, _rd, _rd, 0xF0, arm_imm_lsl_to_rot(24));                 \
  ARM_AND_REG_IMM(0, reg_flags, reg_flags, 0xF0, arm_imm_lsl_to_rot(24));     \
  ARM_ORR_REG_REG(0, _rd, _rd, reg_flags);                                    \
  complete_store_reg(_rd, rd)                                                 \

#define arm_psr_read_spsr()                                                   \
  generate_function_call(execute_read_spsr)                                   \
  generate_store_reg(reg_a0, rd)                                              \

#define arm_psr_read(op_type, psr_reg)                                        \
  arm_psr_read_##psr_reg()                                                    \

// This function's okay because it's called from an ASM function that can
// wrap it correctly.

u32 execute_store_cpsr_body(u32 _cpsr, u32 store_mask, u32 address)
{
  if(store_mask & 0xFF)
  {
    armcpu_switchMode(dynarec_cpu, _cpsr & 0x1F);
    dynarec_cpu->changeCPSR();
    /*if(exec_interrupts(proc))
    {
      reg_mode[MODE_IRQ][6] = address + 4;
      spsr[MODE_IRQ] = _cpsr;
      reg[REG_CPSR] = 0xD2;
      set_cpu_mode(MODE_IRQ);
      return 0xFFFF0018;
    }*/

  }

  dynarec_cpu->R[REG_CPSR] = _cpsr;

  return 0;
}

#define arm_psr_load_new_reg()                                                \
  generate_load_reg(reg_a0, rm)                                               \

#define arm_psr_load_new_imm()                                                \
  generate_load_imm(reg_a0, imm, imm_ror)                                     \

#define arm_psr_store_cpsr()                                                  \
  arm_load_imm_32bit(reg_a1, psr_masks[psr_field]);                           \
  generate_function_call(execute_store_cpsr);                                 \
  write32(pc)                                                                 \

#define arm_psr_store_spsr()                                                  \
  generate_function_call(execute_store_spsr)                                  \

#define arm_psr_store(op_type, psr_reg)                                       \
  arm_psr_load_new_##op_type();                                               \
  arm_psr_store_##psr_reg()                                                   \


#define arm_psr(op_type, transfer_type, psr_reg)                              \
{                                                                             \
	  exec_cyc=1;\
  arm_decode_psr_##op_type();                                                 \
  arm_psr_##transfer_type(op_type, psr_reg);                                  \
}                                                                             \

// TODO: loads will need the PC passed as well for open address, however can
// eventually be rectified with a hash table on the memory accesses
// (same with the stores)

#define load_u8_cycle()\
	MMU_aluMemAccessCycles<proc,8,MMU_AD_READ>(3,pc)\

#define load_u16_cycle()\
		MMU_aluMemAccessCycles<proc,16,MMU_AD_READ>(3,pc)\

#define load_s16_cycle()\
		MMU_aluMemAccessCycles<proc,16,MMU_AD_READ>(3,pc)\

#define load_u32_cycle()\
		MMU_aluMemAccessCycles<proc,32,MMU_AD_READ>(3,pc)\

#define arm_access_memory_load(mem_type, offset_type, adjust_op, direction)   \
  generate_function_call(execute_load_##mem_type);                            \
  write32((pc + 8));                                                          \
  generate_store_reg_pc_no_flags(reg_rv, rd)                                  \

#define arm_access_memory_loadd(mem_type, offset_type, adjust_op, direction)  \
  ARM_MOV_REG_REG(0, reg_s0, reg_a0);   \
  generate_function_call(execute_load_u32);                            \
  write32((pc + 8));                                                          \
  generate_store_reg_pc_no_flags(reg_rv, rd);                                  \
  ARM_ADD_REG_IMM(0, reg_a0, reg_s0, 4, 0);  \
  generate_function_call(execute_load_u32);                            \
  write32((pc + 8));                                                          \
  generate_store_reg_pc_no_flags(reg_rv, rd+1)                                  \

//#define arm_access_memory_store(mem_type, offset_type, adjust_op, direction)  \
//  generate_load_reg_pc(reg_a1, rd, 12);                                       \
//  generate_function_call(execute_store_##mem_type);                           \
//  write32((pc + 4))                                                           \
//
//#define arm_access_memory_stored(mem_type, offset_type, adjust_op, direction) \
//  generate_load_reg_pc(reg_a1, rd, 12);                                       \
//  ARM_MOV_REG_REG(0, reg_s0, reg_a0);   \
//  generate_function_call(execute_store_u32);                           \
//  write32((pc + 4));                                                           \
//  ARM_ADD_REG_IMM(0, reg_a0, reg_s0, 4, 0);  \
//  generate_load_reg_pc(reg_a1, rd+1, 12);                                       \
//  generate_function_call(execute_store_u32);                           \
//  write32((pc + 4))                                                           \

#define arm_access_memory_store(mem_type, offset_type, adjust_op, direction)  \
  generate_load_reg_pc(reg_a1, rd, 8);                                       \
  generate_function_call(execute_store_##mem_type);                           \
  write32((pc + 4))                                                           \

#define arm_access_memory_stored(mem_type, offset_type, adjust_op, direction) \
  generate_load_reg_pc(reg_a1, rd, 8);                                       \
  ARM_MOV_REG_REG(0, reg_s0, reg_a0);   \
  generate_function_call(execute_store_u32);                           \
  write32((pc + 4));                                                           \
  ARM_ADD_REG_IMM(0, reg_a0, reg_s0, 4, 0);  \
  generate_load_reg_pc(reg_a1, rd+1, 8);                                       \
  generate_function_call(execute_store_u32);                           \
  write32((pc + 4))

// Calculate the address into a0 from _rn, _rm

#define arm_access_memory_adjust_reg_sh_up(ireg)                              \
  ARM_ADD_REG_IMMSHIFT(0, ireg, _rn, _rm, ((opcode >> 5) & 0x03),             \
   ((opcode >> 7) & 0x1F))                                                    \

#define arm_access_memory_adjust_reg_sh_down(ireg)                            \
  ARM_SUB_REG_IMMSHIFT(0, ireg, _rn, _rm, ((opcode >> 5) & 0x03),             \
   ((opcode >> 7) & 0x1F))                                                    \

#define arm_access_memory_adjust_reg_up(ireg)                                 \
  ARM_ADD_REG_REG(0, ireg, _rn, _rm)                                          \

#define arm_access_memory_adjust_reg_down(ireg)                               \
  ARM_SUB_REG_REG(0, ireg, _rn, _rm)                                          \

#define arm_access_memory_adjust_imm(op, ireg)                                \
{                                                                             \
  u32 stores[4];                                                              \
  u32 rotations[4];                                                           \
  u32 store_count = arm_disect_imm_32bit(offset, stores, rotations);          \
                                                                              \
  if(store_count > 1)                                                         \
  {                                                                           \
    ARM_##op##_REG_IMM(0, ireg, _rn, stores[0], rotations[0]);                \
    ARM_##op##_REG_IMM(0, ireg, ireg, stores[1], rotations[1]);               \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    ARM_##op##_REG_IMM(0, ireg, _rn, stores[0], rotations[0]);                \
  }                                                                           \
}                                                                             \

#define arm_access_memory_adjust_imm_up(ireg)                                 \
  arm_access_memory_adjust_imm(ADD, ireg)                                     \

#define arm_access_memory_adjust_imm_down(ireg)                               \
  arm_access_memory_adjust_imm(SUB, ireg)                                     \


#define arm_access_memory_pre(type, direction)                                \
  arm_access_memory_adjust_##type##_##direction(reg_a0)                       \

#define arm_access_memory_pre_wb(type, direction)                             \
  arm_access_memory_adjust_##type##_##direction(reg_a0);                      \
  generate_store_reg(reg_a0, rn)                                              \

#define arm_access_memory_post(type, direction)                               \
  u32 _rn_dest = prepare_store_reg(reg_a1, rn);                               \
  if(_rn != reg_a0)                                                           \
  {                                                                           \
    generate_load_reg(reg_a0, rn);                                            \
  }                                                                           \
  arm_access_memory_adjust_##type##_##direction(_rn_dest);                    \
  complete_store_reg(_rn_dest, rn)                                            \


#define arm_data_trans_reg(adjust_op, direction)                              \
  arm_decode_data_trans_reg();                                                \
  u32 _rn = prepare_load_reg_pc(reg_a0, rn, 8);                               \
  u32 _rm = prepare_load_reg(reg_a1, rm);                                     \
  arm_access_memory_##adjust_op(reg_sh, direction)                            \

#define arm_data_trans_imm(adjust_op, direction)                              \
  arm_decode_data_trans_imm();                                                \
  u32 _rn = prepare_load_reg_pc(reg_a0, rn, 8);                               \
  arm_access_memory_##adjust_op(imm, direction)                               \


#define arm_data_trans_half_reg(adjust_op, direction)                         \
  arm_decode_half_trans_r();                                                  \
  u32 _rn = prepare_load_reg_pc(reg_a0, rn, 8);                               \
  u32 _rm = prepare_load_reg(reg_a1, rm);                                     \
  arm_access_memory_##adjust_op(reg, direction)                               \

#define arm_data_trans_half_imm(adjust_op, direction)                         \
  arm_decode_half_trans_of();                                                 \
  u32 _rn = prepare_load_reg_pc(reg_a0, rn, 8);                               \
  arm_access_memory_##adjust_op(imm, direction)                               \


#define arm_access_memory(access_type, direction, adjust_op, mem_type,        \
 offset_type)                                                                 \
{                                                                             \
  arm_data_trans_##offset_type(adjust_op, direction);                         \
  arm_access_memory_##access_type(mem_type, offset_type, adjust_op, direction);                                  \
}                                                                             \


#define word_bit_count(word)                                                  \
  (bit_count[word >> 8] + bit_count[word & 0xFF])                             \

#define sprint_no(access_type, pre_op, post_op, wb)                           \

#define sprint_yes(access_type, pre_op, post_op, wb)                          \
  printf("sbit on %s %s %s %s\n", #access_type, #pre_op, #post_op, #wb)       \


// TODO: Make these use cached registers. Implement iwram_stack_optimize.

#define arm_block_memory_load()                                               \
  generate_function_call(execute_load_u32);                                   \
  write32((pc + 8));                                                          \
  generate_store_reg(reg_rv, i)                                               \

#define arm_block_memory_store()                                              \
  generate_load_reg_pc(reg_a1, i, 8);                                         \
  generate_function_call(execute_store_u32_safe)                              \

#define arm_block_memory_final_load()                                         \
  arm_block_memory_load()                                                     \

//#define arm_block_memory_final_store()                                        \
//  generate_load_reg_pc(reg_a1, i, 12);                                        \
//  generate_function_call(execute_store_u32);                                  \
//  write32((pc + 4))                                                           \

#define arm_block_memory_final_store()                                        \
  generate_load_reg_pc(reg_a1, i, 8);                                        \
  generate_function_call(execute_store_u32);                                  \
  write32((pc + 4))                                                           \

#define arm_block_memory_adjust_pc_store()                                    \

#define arm_block_memory_adjust_pc_load()                                     \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    generate_mov(reg_a0, reg_rv);                                             \
    generate_indirect_branch_dual();                                           \
  }                                                                           \

#define arm_block_memory_offset_down_a()                                      \
  generate_sub_imm(reg_s0, ((word_bit_count(reg_list) * 4) - 4), 0)           \

#define arm_block_memory_offset_down_b()                                      \
  generate_sub_imm(reg_s0, (word_bit_count(reg_list) * 4), 0)                 \

#define arm_block_memory_offset_no()                                          \

#define arm_block_memory_offset_up()                                          \
  generate_add_imm(reg_s0, 4, 0)                                              \

#define arm_block_memory_writeback_down()                                     \
  generate_load_reg(reg_a0, rn);                                              \
  generate_sub_imm(reg_a0, (word_bit_count(reg_list) * 4), 0);                \
  generate_store_reg(reg_a0, rn)                                              \

#define arm_block_memory_writeback_up()                                       \
  generate_load_reg(reg_a0, rn);                                              \
  generate_add_imm(reg_a0, (word_bit_count(reg_list) * 4), 0);                \
  generate_store_reg(reg_a0, rn)                                              \

#define arm_block_memory_writeback_no()

// Only emit writeback if the register is not in the list

#define arm_block_memory_writeback_load(writeback_type)                       \
  if(!((reg_list >> rn) & 0x01))                                              \
  {                                                                           \
    arm_block_memory_writeback_##writeback_type();                            \
  }                                                                           \

#define arm_block_memory_writeback_store(writeback_type)                      \
  arm_block_memory_writeback_##writeback_type()                               \

extern void switch_user();
extern void switch_back();


#define arm_block_memory(access_type, offset_type, writeback_type, s_bit)     \
{                                                                             \
  arm_decode_block_trans();                                                   \
  u32 offset = 0;                                                             \
  u32 i;                                                                      \
  u32 register15 = (opcode >>15) &0x01;\
     \
                                                                              \
  generate_load_reg(reg_s0, rn);                                              \
  arm_block_memory_offset_##offset_type();                                    \
  arm_block_memory_writeback_##access_type(writeback_type);                   \
  ARM_BIC_REG_IMM(0, reg_s0, reg_s0, 0x03, 0);                                \
  \
if(((opcode >> 22) & 0x01) && !(register15 && ((opcode >> 20) & 0x01)))  \
{generate_function_call(switch_user);\
}\
\
                                                                              \
  for(i = 0; i < 16; i++)                                                     \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      generate_add_reg_reg_imm(reg_a0, reg_s0, offset, 0);                    \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        arm_block_memory_##access_type();                                     \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        arm_block_memory_final_##access_type();                               \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  }                                                                           \
  if((opcode >> 22) & 0x01)  \
  {\
	  if(register15 && (opcode>>20 & 0x01))  \
	  {   \
		  generate_function_call(execute_spsr_restore);\
	  }   \
	  else   \
	  {  \
		  generate_function_call(switch_back);   \
	  }   \
  }\
   arm_block_memory_adjust_pc_##access_type();                                 \
                                                                              \
 \
  \
}                                                                             \

#define arm_swap(type)                                                        \
{                                                                             \
  arm_decode_swap();                                                          \
  generate_load_reg(reg_a0, rn);                                              \
  generate_function_call(execute_load_##type);                                \
  write32((pc + 8));                                                          \
  generate_mov(reg_s0, reg_rv);                                               \
  generate_load_reg(reg_a0, rn);                                              \
  generate_load_reg(reg_a1, rm);                                              \
  generate_function_call(execute_store_##type);                               \
  write32((pc + 4));                                                          \
  generate_store_reg(reg_s0, rd);                                             \
}                                                                             \



#define thumb_generate_op_reg(name, _rd, _rs, _rn)                            \
  u32 __rm = prepare_load_reg(reg_rm, _rn);                                   \
  generate_op_##name##_reg_immshift(__rd, __rn, __rm, ARMSHIFT_LSL, 0)        \

#define thumb_generate_op_imm(name, _rd, _rs, imm_)                           \
{                                                                             \
  u32 imm_ror = 0;                                                            \
  generate_op_##name##_imm(__rd, __rn);                                       \
}                                                                             \


#define thumb_data_proc(type, name, op_type, _rd, _rs, _rn)                   \
{                                                                             \
	exec_cyc = 1 ;\
  thumb_decode_##type();                                                      \
  u32 __rn = prepare_load_reg(reg_rn, _rs);                                   \
  u32 __rd = prepare_store_reg(reg_rd, _rd);                                  \
  generate_load_reg(reg_rn, _rs);                                             \
  thumb_generate_op_##op_type(name, _rd, _rs, _rn);                           \
  complete_store_reg(__rd, _rd);                                              \
}                                                                             \

#define thumb_data_proc_test(type, name, op_type, _rd, _rs)                   \
{                                                                             \
	exec_cyc = 1 ;\
  thumb_decode_##type();                                                      \
  u32 __rn = prepare_load_reg(reg_rn, _rd);                                   \
  thumb_generate_op_##op_type(name, 0, _rd, _rs);                             \
}                                                                             \

#define thumb_data_proc_unary(type, name, op_type, _rd, _rs)                  \
{                                                                             \
	exec_cyc = 1 ;\
  thumb_decode_##type();                                                      \
  u32 __rd = prepare_store_reg(reg_rd, _rd);                                  \
  thumb_generate_op_##op_type(name, _rd, 0, _rs);                             \
  complete_store_reg(__rd, _rd);                                              \
}                                                                             \


#define complete_store_reg_pc_thumb()                                         \
  if(rd == 15)                                                                \
  {                                                                           \
    generate_indirect_branch_cycle_update(thumb);                             \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    complete_store_reg(_rd, rd);                                              \
  }                                                                           \

#define thumb_data_proc_hi(name)                                              \
{                                                                             \
	  exec_cyc = 1;\
  thumb_decode_hireg_op();                                                    \
  u32 _rd = prepare_load_reg_pc(reg_rd, rd, 4);                               \
  u32 _rs = prepare_load_reg_pc(reg_rn, rs, 4);                               \
  generate_op_##name##_reg_immshift(_rd, _rd, _rs, ARMSHIFT_LSL, 0);          \
  complete_store_reg_pc_thumb();                                              \
}                                                                             \

#define thumb_data_proc_test_hi(name)                                         \
{                                                                             \
	  exec_cyc = 1;\
  thumb_decode_hireg_op();                                                    \
  u32 _rd = prepare_load_reg_pc(reg_rd, rd, 4);                               \
  u32 _rs = prepare_load_reg_pc(reg_rn, rs, 4);                               \
  generate_op_##name##_reg_immshift(0, _rd, _rs, ARMSHIFT_LSL, 0);            \
}                                                                             \

#define thumb_data_proc_mov_hi()                                              \
{                                                                             \
	  exec_cyc = 1;\
  thumb_decode_hireg_op();                                                    \
  u32 _rs = prepare_load_reg_pc(reg_rn, rs, 4);                               \
  u32 _rd = prepare_store_reg(reg_rd, rd);                                    \
  ARM_MOV_REG_REG(0, _rd, _rs);                                               \
  complete_store_reg_pc_thumb();                                              \
}                                                                             \



#define thumb_load_pc(_rd)                                                    \
{                                                                             \
	exec_cyc = 1 ;\
  thumb_decode_imm();                                                         \
  u32 __rd = prepare_store_reg(reg_rd, _rd);                                  \
  generate_load_pc(__rd, (((pc & ~2) + 4) + (imm * 4)));                      \
  complete_store_reg(__rd, _rd);                                              \
}                                                                             \

#define thumb_load_sp(_rd)                                                    \
{                                                                             \
	exec_cyc=1;\
  thumb_decode_imm();                                                         \
  u32 __sp = prepare_load_reg(reg_a0, REG_SP);                                \
  u32 __rd = prepare_store_reg(reg_a0, _rd);                                  \
  ARM_ADD_REG_IMM(0, __rd, __sp, imm, arm_imm_lsl_to_rot(2));                 \
  complete_store_reg(__rd, _rd);                                              \
}                                                                             \

#define thumb_adjust_sp_up()                                                  \
  ARM_ADD_REG_IMM(0, _sp, _sp, imm, arm_imm_lsl_to_rot(2))                    \

#define thumb_adjust_sp_down()                                                \
  ARM_SUB_REG_IMM(0, _sp, _sp, imm, arm_imm_lsl_to_rot(2))                    \

#define thumb_adjust_sp(direction)                                            \
{                                                                             \
	exec_cyc=1; \
  thumb_decode_add_sp();                                                      \
  u32 _sp = prepare_load_reg(reg_a0, REG_SP);                                 \
  thumb_adjust_sp_##direction();                                              \
  complete_store_reg(_sp, REG_SP);                                            \
}                                                                             \

#define generate_op_lsl_reg(_rd, _rm, _rs)                                    \
	exec_cyc=2; \
  generate_op_movs_reg_regshift(_rd, 0, _rm, ARMSHIFT_LSL, _rs)               \

#define generate_op_lsr_reg(_rd, _rm, _rs)                                    \
	exec_cyc=2; \
  generate_op_movs_reg_regshift(_rd, 0, _rm, ARMSHIFT_LSR, _rs)               \

#define generate_op_asr_reg(_rd, _rm, _rs)                                    \
	exec_cyc=2; \
  generate_op_movs_reg_regshift(_rd, 0, _rm, ARMSHIFT_ASR, _rs)               \

#define generate_op_ror_reg(_rd, _rm, _rs)                                    \
	exec_cyc=2; \
  generate_op_movs_reg_regshift(_rd, 0, _rm, ARMSHIFT_ROR, _rs)               \


#define generate_op_lsl_imm(_rd, _rm)                                         \
	exec_cyc=1; \
  generate_op_movs_reg_immshift(_rd, 0, _rm, ARMSHIFT_LSL, imm)               \

#define generate_op_lsr_imm(_rd, _rm)                                         \
	exec_cyc=1; \
  generate_op_movs_reg_immshift(_rd, 0, _rm, ARMSHIFT_LSR, imm)               \

#define generate_op_asr_imm(_rd, _rm)                                         \
	exec_cyc=1; \
  generate_op_movs_reg_immshift(_rd, 0, _rm, ARMSHIFT_ASR, imm)               \

#define generate_op_ror_imm(_rd, _rm)                                         \
	exec_cyc=1; \
  generate_op_movs_reg_immshift(_rd, 0, _rm, ARMSHIFT_ROR, imm)               \


#define generate_shift_reg(op_type)                                           \
  u32 __rm = prepare_load_reg(reg_rd, rd);                                    \
  u32 __rs = prepare_load_reg(reg_rs, rs);                                    \
  generate_op_##op_type##_reg(__rd, __rm, __rs)                               \

#define generate_shift_imm(op_type)                                           \
  u32 __rs = prepare_load_reg(reg_rs, rs);                                    \
  generate_op_##op_type##_imm(__rd, __rs)                                     \


#define thumb_shift(decode_type, op_type, value_type)                         \
{                                                                             \
  thumb_decode_##decode_type();                                               \
  u32 __rd = prepare_store_reg(reg_rd, rd);                                   \
  generate_shift_##value_type(op_type);                                       \
  complete_store_reg(__rd, rd);                                               \
}                                                                             \

// Operation types: imm, mem_reg, mem_imm

#define thumb_access_memory_load(mem_type, _rd)                               \
  generate_function_call(execute_load_##mem_type);                            \
  write32((pc + 4));                                                          \
  generate_store_reg(reg_rv, _rd)                                             \

#define thumb_access_memory_store(mem_type, _rd)                              \
  generate_load_reg(reg_a1, _rd);                                             \
  generate_function_call(execute_store_##mem_type);                           \
  write32((pc + 2))                                                           \

#define thumb_access_memory_generate_address_pc_relative(offset, _rb, _ro)    \
  generate_load_pc(reg_a0, (offset))                                          \

#define thumb_access_memory_generate_address_reg_imm(offset, _rb, _ro)        \
  u32 __rb = prepare_load_reg(reg_a0, _rb);                                   \
  ARM_ADD_REG_IMM(0, reg_a0, __rb, offset, 0)                                 \

#define thumb_access_memory_generate_address_reg_imm_sp(offset, _rb, _ro)     \
  u32 __rb = prepare_load_reg(reg_a0, _rb);                                   \
  ARM_ADD_REG_IMM(0, reg_a0, __rb, offset, arm_imm_lsl_to_rot(2))             \

#define thumb_access_memory_generate_address_reg_reg(offset, _rb, _ro)        \
  u32 __rb = prepare_load_reg(reg_a0, _rb);                                   \
  u32 __ro = prepare_load_reg(reg_a1, _ro);                                   \
  ARM_ADD_REG_REG(0, reg_a0, __rb, __ro)                                      \

#define thumb_access_memory(access_type, op_type, _rd, _rb, _ro,              \
 address_type, offset, mem_type)                                              \
{                                                                             \
  thumb_decode_##op_type();                                                   \
  thumb_access_memory_generate_address_##address_type(offset, _rb, _ro);      \
  thumb_access_memory_##access_type(mem_type, _rd);                           \
}                                                                             \

// TODO: Make these use cached registers. Implement iwram_stack_optimize.

#define thumb_block_address_preadjust_up()                                    \
  generate_add_imm(reg_s0, (bit_count[reg_list] * 4), 0)                      \

#define thumb_block_address_preadjust_down()                                  \
  generate_sub_imm(reg_s0, (bit_count[reg_list] * 4), 0)                      \

#define thumb_block_address_preadjust_push_lr()                               \
  generate_sub_imm(reg_s0, ((bit_count[reg_list] + 1) * 4), 0)                \

#define thumb_block_address_preadjust_no()                                    \

#define thumb_block_address_postadjust_no(base_reg)                           \
  generate_store_reg(reg_s0, base_reg)                                        \

#define thumb_block_address_postadjust_up(base_reg)                           \
  generate_add_reg_reg_imm(reg_a0, reg_s0, (bit_count[reg_list] * 4), 0);     \
  generate_store_reg(reg_a0, base_reg)                                        \

#define thumb_block_address_postadjust_down(base_reg)                         \
  generate_mov(reg_a0, reg_s0);                                               \
  generate_sub_imm(reg_a0, (bit_count[reg_list] * 4), 0);                     \
  generate_store_reg(reg_a0, base_reg)                                        \

#define thumb_block_address_postadjust_pop_pc(base_reg)                       \
  generate_add_reg_reg_imm(reg_a0, reg_s0,                                    \
   ((bit_count[reg_list] + 1) * 4), 0);                                       \
  generate_store_reg(reg_a0, base_reg)                                        \

#define thumb_block_address_postadjust_push_lr(base_reg)                      \
  generate_store_reg(reg_s0, base_reg)                                        \

#define thumb_block_memory_extra_no()                                         \

#define thumb_block_memory_extra_up()                                         \

#define thumb_block_memory_extra_down()                                       \

#define thumb_block_memory_extra_pop_pc()                                     \
  generate_add_reg_reg_imm(reg_a0, reg_s0, (bit_count[reg_list] * 4), 0);     \
  generate_function_call(execute_load_u32);                                   \
  write32((pc + 4));                                                          \
  generate_mov(reg_a0, reg_rv);                                               \
  generate_indirect_branch_cycle_update(dual_thumb)                                \

#define thumb_block_memory_extra_push_lr(base_reg)                            \
  generate_add_reg_reg_imm(reg_a0, reg_s0, (bit_count[reg_list] * 4), 0);     \
  generate_load_reg(reg_a1, REG_LR);                                          \
  generate_function_call(execute_store_u32_safe)                              \

#define thumb_block_memory_load()                                             \
  generate_function_call(execute_load_u32);                                   \
  write32((pc + 4));                                                          \
  generate_store_reg(reg_rv, i)                                               \

#define thumb_block_memory_store()                                            \
  generate_load_reg(reg_a1, i);                                               \
  generate_function_call(execute_store_u32_safe)                              \

#define thumb_block_memory_final_load()                                       \
  thumb_block_memory_load()                                                   \

#define thumb_block_memory_final_store()                                      \
  generate_load_reg(reg_a1, i);                                               \
  generate_function_call(execute_store_u32);                                  \
  write32((pc + 2))                                                           \

#define thumb_block_memory_final_no(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_up(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_down(access_type)                            \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_push_lr(access_type)                         \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory_final_pop_pc(access_type)                          \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory(access_type, pre_op, post_op, base_reg)            \
{                                                                             \
  thumb_decode_rlist();                                                       \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
                                                                              \
  generate_load_reg(reg_s0, base_reg);                                        \
  ARM_BIC_REG_IMM(0, reg_s0, reg_s0, 0x03, 0);                                \
  thumb_block_address_preadjust_##pre_op();                                   \
  thumb_block_address_postadjust_##post_op(base_reg);                         \
                                                                              \
  for(i = 0; i < 8; i++)                                                      \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      generate_add_reg_reg_imm(reg_a0, reg_s0, offset, 0);                    \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        thumb_block_memory_##access_type();                                   \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        thumb_block_memory_final_##post_op(access_type);                      \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  thumb_block_memory_extra_##post_op();                                       \
}                                                                             \

#define thumb_conditional_branch(condition)                                   \
{                                                                             \
  generate_cycle_update();                                                    \
  generate_load_flags();                                                      \
  generate_branch_filler(condition_opposite_##condition, backpatch_address);  \
  generate_branch_no_cycle_update(                                            \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target, thumb);                    \
  generate_branch_patch_conditional_thumb(backpatch_address, translation_ptr);      \
  block_exit_position++;                                                      \
}                                                                             \


#define arm_conditional_block_header()                                        \
  generate_cycle_update();                                                    \
  generate_load_flags();                                                      \
  /* This will choose the opposite condition */                               \
  condition ^= 0x01;                                                          \
  generate_branch_filler(condition, backpatch_address)                        \

#define arm_b()                                                               \
		cycle_count+=3;\
  generate_branch(arm)                                                        \

#define arm_bl()                                                              \
	cycle_count+=3;\
  generate_update_pc((pc + 4));                                               \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_branch(arm)                                                        \

#define arm_bx()                                                              \
		cycle_count+=3;\
  arm_decode_branchx();                                                       \
  generate_load_reg(reg_a0, rn);                                              \
  generate_indirect_branch_dual();                                            \

extern void arm_update_gba_to_thumb();
extern void arm_update_gba_idle_to_thumb();

extern void to_thumb_handler();
extern void to_arm_handler();

#define arm_blx_imm()\
		cycle_count+=3;\
  generate_update_pc((pc + 4));                                               \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_function_call(to_thumb_handler); \
  generate_branch_blx_imm(thumb)                                                        \

#define arm_blx_reg()\
		cycle_count+=3;\
arm_decode_branchx();                                                       \
generate_update_pc((pc + 4));                                               \
 generate_store_reg(reg_a0, REG_LR);                                         \
  generate_load_reg(reg_a0, rn);                                              \
  generate_indirect_branch_dual();                                            \

#define arm_clz()\
{	exec_cyc=2;\
	arm_decode_clz();\
	u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
	generate_load_reg(_rd, rm);                                           \
	write32(0xe16f0f10 | _rd | (_rd<<12));\
	complete_store_reg(_rd, rd);                                                 }\

#define generate_q_add(_rn, _rd, _rm)\
	write32(0xe1000050 | (_rn << 16) | (_rd << 12) | (_rm))\

#define generate_q_dadd(_rn, _rd, _rm)  \
	write32(0xe1400050 | (_rn <<16) | (_rd << 12) |(_rm))  \

#define generate_q_sub(_rn, _rd, _rm)\
	write32(0xe1200050 | (_rn << 16) | (_rd << 12) | (_rm))\

#define generate_q_dsub(_rn, _rd, _rm)\
	write32(0xe1600050 | (_rn << 16) | (_rd << 12) | (_rm))\

#define arm_q_op(name) \
{                                                                             \
  arm_decode_q_op();                                                      \
  exec_cyc = rd ==15 ? 3 : 2;\
  u32 _rm = prepare_load_reg(reg_a0, rm);                                     \
  u32 _rn = prepare_load_reg(reg_a1, rn);                                     \
  u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
  generate_load_flags();                                                      \
    generate_q_##name(_rn, _rd, _rm);                                         \
    generate_store_flags();                                                      \
  complete_store_reg(_rd, rd);                                                \
}                                                                             \


#define arm_q_mla()\
		{                                                                             \
	exec_cyc=2;\
		  arm_decode_q_mla();                                                      \
		  u32 _rm = prepare_load_reg(reg_a0, rm);                                     \
		  u32 _rn = prepare_load_reg(reg_a1, rn);                                     \
		  u32 _rs = prepare_load_reg(reg_a2, rs);                                   \
		  u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
		  generate_load_flags();                                                      \
		    write32(clean_op | (_rd << 16) | (_rn << 12) | (_rs << 8) | _rm  | 0xE0000000);                                         \
		    generate_store_flags();                                                      \
		  complete_store_reg(_rd, rd);                                                \
		}    \

#define arm_q_mul()\
		{                                                                             \
			exec_cyc=2;\
		  arm_decode_q_mul();                                                      \
		  u32 _rm = prepare_load_reg(reg_a0, rm);                                     \
		  u32 _rs = prepare_load_reg(reg_a1, rs);                                   \
		  u32 _rd = prepare_store_reg(reg_a0, rd);                                    \
		  generate_load_flags();                                                      \
		    write32(clean_op | (_rd << 16) | (_rs << 8) | _rm | 0xE0000000);                                         \
		    generate_store_flags();                                                      \
		  complete_store_reg(_rd, rd);                                                \
		}    \


extern void arm_mcr_asm(const u32 i);
extern void arm_mrc_asm(const u32 i);

#define arm_mcr_emit()\
		cycle_count+= 2;                                                           \
		generate_cycle_update(); \
		generate_function_call(arm_mcr_asm);                            \
		write32((pc + 4));                                                          \

#define arm_mrc_emit()\
		exec_cyc = 4;                                                           \
		generate_function_call(arm_mrc_asm);                            \
		write32((pc + 4));                                                          \

#define arm_swi()                                                             \
	generate_cycle_update(); \
  generate_function_call(execute_swi_arm);                                    \
  write32((pc + 4));                                                          \


#define thumb_b()                                                             \
		cycle_count+=1; \
  generate_branch(thumb)                                                      \

#define thumb_bl()                                                            \
		cycle_count+=5;\
  generate_update_pc(((pc + 2) | 0x01));                                      \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_branch(thumb)                                                      \

#define thumb_blh()                                                           \
{                                                                             \
	  cycle_count+=1;\
  thumb_decode_branch();                                                      \
  generate_update_pc(((pc + 2) | 0x01));                                      \
  generate_load_reg(reg_a1, REG_LR);                                          \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_mov(reg_a0, reg_a1);                                               \
  generate_add_imm(reg_a0, (offset * 2), 0);                                  \
  generate_indirect_branch_cycle_update(thumb);                               \
}                                                                             \

extern void arm_update_gba_to_arm();
extern void arm_update_gba_idle_to_arm();
extern void arm_indirect_branch_to_arm();

#define thumb_blx_imm()                                                           \
{                                                                             \
	/*block_exits[block_exit_position].branch_source  */                          \
		cycle_count+=4;\
  generate_update_pc(((pc + 2)|0x01));                                      \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_function_call(to_arm_handler); \
  generate_branch_blx_imm(arm);                          \
}                                                                             \

#define thumb_blxh()                                                           \
{                                                                             \
	  cycle_count+=1;\
  thumb_decode_branch();                                                      \
  generate_update_pc(((pc + 2) | 0x01));                                      \
  generate_load_reg(reg_a1, REG_LR);                                          \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_mov(reg_a0, reg_a1);                                               \
  generate_add_imm(reg_a0, (offset * 2), 0);                                  \
  generate_indirect_branch_cycle_update(to_arm);                               \
}                                                                             \

#define thumb_bx()                                                            \
{                                                                             \
	cycle_count+=3;\
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(reg_a0, rs, 4);                                        \
  generate_indirect_branch_cycle_update(dual_thumb);                          \
}                                                                             \

#define thumb_blx_reg()                                                            \
{                                                                             \
	cycle_count+=4;\
  thumb_decode_hireg_op();                                                    \
  generate_update_pc(((pc + 2)|0x01)); \
  generate_store_reg(reg_a0, REG_LR);                                         \
  generate_load_reg_pc(reg_a0, rs, 2);                                        \
  generate_indirect_branch_cycle_update(dual_thumb);                          \
}                                                                             \


#define thumb_swi()                                                           \
	generate_cycle_update(); \
  generate_function_call(execute_swi_thumb);                                  \
  write32((pc + 2));                                                          \

u8 swi_hle_handle[256] =
{
  0x0,    // SWI 0:  SoftReset
  0x0,    // SWI 1:  RegisterRAMReset
  0x0,    // SWI 2:  Halt
  0x0,    // SWI 3:  Stop/Sleep
  0x0,    // SWI 4:  IntrWait
  0x0,    // SWI 5:  VBlankIntrWait
  0x1,    // SWI 6:  Div
  0x0,    // SWI 7:  DivArm
  0x0,    // SWI 8:  Sqrt
  0x0,    // SWI 9:  ArcTan
  0x0,    // SWI A:  ArcTan2
  0x0,    // SWI B:  CpuSet
  0x0,    // SWI C:  CpuFastSet
  0x0,    // SWI D:  GetBIOSCheckSum
  0x0,    // SWI E:  BgAffineSet
  0x0,    // SWI F:  ObjAffineSet
  0x0,    // SWI 10: BitUnpack
  0x0,    // SWI 11: LZ77UnCompWram
  0x0,    // SWI 12: LZ77UnCompVram
  0x0,    // SWI 13: HuffUnComp
  0x0,    // SWI 14: RLUnCompWram
  0x0,    // SWI 15: RLUnCompVram
  0x0,    // SWI 16: Diff8bitUnFilterWram
  0x0,    // SWI 17: Diff8bitUnFilterVram
  0x0,    // SWI 18: Diff16bitUnFilter
  0x0,    // SWI 19: SoundBias
  0x0,    // SWI 1A: SoundDriverInit
  0x0,    // SWI 1B: SoundDriverMode
  0x0,    // SWI 1C: SoundDriverMain
  0x0,    // SWI 1D: SoundDriverVSync
  0x0,    // SWI 1E: SoundChannelClear
  0x0,    // SWI 1F: MidiKey2Freq
  0x0,    // SWI 20: SoundWhatever0
  0x0,    // SWI 21: SoundWhatever1
  0x0,    // SWI 22: SoundWhatever2
  0x0,    // SWI 23: SoundWhatever3
  0x0,    // SWI 24: SoundWhatever4
  0x0,    // SWI 25: MultiBoot
  0x0,    // SWI 26: HardReset
  0x0,    // SWI 27: CustomHalt
  0x0,    // SWI 28: SoundDriverVSyncOff
  0x0,    // SWI 29: SoundDriverVSyncOn
  0x0     // SWI 2A: SoundGetJumpList
};

void execute_swi_hle_div_arm();
void execute_swi_hle_div_thumb();

void execute_swi_hle_div_c()
{
  /*s32 result = (s32)reg[0] / (s32)reg[1];
  reg[1] = (s32)reg[0] % (s32)reg[1];
  reg[0] = result;

  reg[3] = (result ^ (result >> 31)) - (result >> 31);*/
}

#define generate_swi_hle_handler(_swi_number, mode)                           \
{                                                                             \
  u32 swi_number = _swi_number;                                               \
  if(swi_hle_handle[swi_number])                                              \
  {                                                                           \
    /* Div */                                                                 \
    if(swi_number == 0x06)                                                    \
    {                                                                         \
      generate_function_call(execute_swi_hle_div_##mode);                     \
    }                                                                         \
    break;                                                                    \
  }                                                                           \
}                                                                             \

#define generate_translation_gate(type)                                       \
  generate_update_pc(pc);                                                     \
  generate_indirect_branch_no_cycle_update(type)                              \

#define generate_step_debug()                                                 \
  generate_function_call(step_debug_arm);                                     \
  write32(pc)                                                                 \

#endif


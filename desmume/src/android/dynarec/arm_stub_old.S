.align 2

.extern block_lookup_address_arm
.extern block_lookup_address_thumb
.extern execute_store_cpsr_body
.extern execute_swi_hle_div_c
.extern move_reg
.extern ds_write8
.extern ds_write16
.extern ds_write32
.extern ds_read8
.extern ds_read16
.extern ds_read32
.extern NDS_update_dynarec
.extern update_dynarec
.extern logmeow
.extern arm_mrc
.extern arm_mcr
.extern switch_user_arm
.extern switch_back_arm

.global switch_user
.global switch_back

.global arm_update_gba_arm
.global arm_update_gba_thumb
.global arm_update_gba_to_arm
.global arm_update_gba_to_thumb
.global arm_update_gba_idle_arm
.global arm_update_gba_idle_thumb
.global arm_update_gba_idle_to_arm
.global arm_update_gba_idle_to_thumb

.global arm_indirect_branch_arm
.global arm_indirect_branch_thumb
.global arm_indirect_branch
.global arm_indirect_branch_dual_arm
.global arm_indirect_branch_dual_thumb
.global arm_indirect_branch_to_arm

.global execute_arm_translate

.global execute_store_u8
.global execute_store_u16
.global execute_store_u32
.global execute_store_u32_safe

.global execute_load_u8
.global execute_load_s8
.global execute_load_u16
.global execute_load_s16
.global execute_load_u32

.global execute_store_cpsr
.global execute_read_spsr
.global execute_store_spsr
.global execute_spsr_restore

.global execute_swi_arm
.global execute_swi_thumb

.global execute_patch_bios_read
.global execute_patch_bios_protect

.global execute_bios_ptr_protected
.global execute_bios_rom_ptr

.global arm_mrc_asm
.global arm_mcr_asm

.global step_debug_arm

.global invalidate_icache_region
.global invalidate_cache_region

.global memory_map_read
.global memory_map_write
.global reg

#define REG_BASE_OFFSET   1024

#define REG_R0            (REG_BASE_OFFSET + (0 * 4))
#define REG_R1            (REG_BASE_OFFSET + (1 * 4))
#define REG_R2            (REG_BASE_OFFSET + (2 * 4))
#define REG_R3            (REG_BASE_OFFSET + (3 * 4))
#define REG_R4            (REG_BASE_OFFSET + (4 * 4))
#define REG_R5            (REG_BASE_OFFSET + (5 * 4))
#define REG_R6            (REG_BASE_OFFSET + (6 * 4))
#define REG_R7            (REG_BASE_OFFSET + (7 * 4))
#define REG_R8            (REG_BASE_OFFSET + (8 * 4))
#define REG_R9            (REG_BASE_OFFSET + (9 * 4))
#define REG_R10           (REG_BASE_OFFSET + (10 * 4))
#define REG_R11           (REG_BASE_OFFSET + (11 * 4))
#define REG_R12           (REG_BASE_OFFSET + (12 * 4))
#define REG_R13           (REG_BASE_OFFSET + (13 * 4))
#define REG_R14           (REG_BASE_OFFSET + (14 * 4))
#define REG_SP            (REG_BASE_OFFSET + (13 * 4))
#define REG_LR            (REG_BASE_OFFSET + (14 * 4))
#define REG_PC            (REG_BASE_OFFSET + (15 * 4))

#define REG_CPSR          (REG_BASE_OFFSET + (20 * 4))

#define REG_SAVE          (REG_BASE_OFFSET + (21 * 4))
#define REG_SAVE2         (REG_BASE_OFFSET + (22 * 4))
#define REG_SAVE3         (REG_BASE_OFFSET + (23 * 4))

#define CPU_MODE          (REG_BASE_OFFSET + (29 * 4))
#define CPU_HALT_STATE    (REG_BASE_OFFSET + (30 * 4))
#define CHANGED_PC_STATUS (REG_BASE_OFFSET + (31 * 4))


#define reg_a0            r0
#define reg_a1            r1
#define reg_a2            r2

#define reg_s0            r9
#define reg_base          sp
#define reg_flags         r11

#define reg_cycles        r12

#define reg_x0            r3
#define reg_x1            r4
#define reg_x2            r5
#define reg_x3            r6
#define reg_x4            r7
#define reg_x5            r8


#define MODE_SUPERVISOR   3

#ifdef __ARM_ARCH_7A__
  #define extract_u16(rd, rs) \
    uxth rd, rs
#else
  #define extract_u16(rd, rs) \
    bic  rd, rs, #0xff000000 ;\
    bic  rd, rd, #0x00ff0000
#endif

@ Will load the register set from memory into the appropriate cached registers.
@ See arm_emit.h for listing explanation.

#define load_registers_arm()                                                 ;\
  ldr reg_x0, [reg_base, #REG_R0]                                            ;\
  ldr reg_x1, [reg_base, #REG_R1]                                            ;\
  ldr reg_x2, [reg_base, #REG_R6]                                            ;\
  ldr reg_x3, [reg_base, #REG_R9]                                            ;\
  ldr reg_x4, [reg_base, #REG_R12]                                           ;\
  ldr reg_x5, [reg_base, #REG_R14]                                           ;\

#define load_registers_thumb()                                               ;\
  ldr reg_x0, [reg_base, #REG_R0]                                            ;\
  ldr reg_x1, [reg_base, #REG_R1]                                            ;\
  ldr reg_x2, [reg_base, #REG_R2]                                            ;\
  ldr reg_x3, [reg_base, #REG_R3]                                            ;\
  ldr reg_x4, [reg_base, #REG_R4]                                            ;\
  ldr reg_x5, [reg_base, #REG_R5]                                            ;\


@ Will store the register set from cached registers back to memory.

#define store_registers_arm()                                                ;\
  str reg_x0, [reg_base, #REG_R0]                                            ;\
  str reg_x1, [reg_base, #REG_R1]                                            ;\
  str reg_x2, [reg_base, #REG_R6]                                            ;\
  str reg_x3, [reg_base, #REG_R9]                                            ;\
  str reg_x4, [reg_base, #REG_R12]                                           ;\
  str reg_x5, [reg_base, #REG_R14]                                           ;\

#define store_registers_thumb()                                              ;\
  str reg_x0, [reg_base, #REG_R0]                                            ;\
  str reg_x1, [reg_base, #REG_R1]                                            ;\
  str reg_x2, [reg_base, #REG_R2]                                            ;\
  str reg_x3, [reg_base, #REG_R3]                                            ;\
  str reg_x4, [reg_base, #REG_R4]                                            ;\
  str reg_x5, [reg_base, #REG_R5]                                            ;\


@ Returns an updated persistent cpsr with the cached flags register.
@ Uses reg as a temporary register and returns the CPSR here.

#define collapse_flags_no_update(reg)                                        ;\
  ldr reg, [reg_base, #REG_CPSR]          /* reg = cpsr                    */;\
  bic reg, reg, #0xF0000000               /* clear ALU flags in cpsr       */;\
  and reg_flags, reg_flags, #0xF0000000   /* clear non-ALU flags           */;\
  orr reg, reg, reg_flags                 /* update cpsr with ALU flags    */;\

@ Updates cpsr using the above macro.

#define collapse_flags(reg)                                                  ;\
  collapse_flags_no_update(reg)                                              ;\
  str reg, [reg_base, #REG_CPSR]                                             ;\

@ Loads the saved flags register from the persistent cpsr.

#define extract_flags()                                                      ;\
  ldr reg_flags, [reg_base, #REG_CPSR]                                       ;\
  msr cpsr_f, reg_flags                                                      ;\


#define save_flags()                                                         ;\
  mrs reg_flags, cpsr                                                        ;\

#define restore_flags()                                                      ;\
  msr cpsr_f, reg_flags                                                      ;\

#ifdef __ARM_EABI__
  @ must align stack
  #define call_c_saved_regs r2, r3, r12, lr
#else
  #define call_c_saved_regs r3, r12, lr
#endif

@ Calls a C function - all caller save registers which are important to the
@ dynarec and to returning from this function are saved.

#define call_c_function(function)                                            ;\
  stmdb sp!, { call_c_saved_regs }                                           ;\
  bl function                                                                ;\
  ldmia sp!, { call_c_saved_regs }                                           ;\


@ Update the GBA hardware (video, sound, input, etc)

@ Input:
@ r0: current PC

#define return_straight()                                                    ;\
  bx lr                                                                      ;\

#define return_add()                                                         ;\
  add pc, lr, #4                                                             ;\

#define load_pc_straight()                                                   ;\
  ldr r0, [lr, #-8]                                                          ;\

#define load_pc_add()                                                        ;\
  ldr r0, [lr]                                                               ;\


#define arm_update_gba_builder(name, mode, return_op)                        ;\
                                                                             ;\
arm_update_gba_##name:                                                       ;\
  load_pc_##return_op()                                                      ;\
  str r0, [reg_base, #REG_PC]             /* write out the PC              */;\
                                                                             ;\
  save_flags()                                                               ;\
  collapse_flags(r0)                      /* update the flags              */;\
                                                                             ;\
  store_registers_##mode()                /* save out registers            */;\
  mov r0, reg_cycles;\
  call_c_function(NDS_update_dynarec)    /*          update GBA state              */;\
                                                                             ;\
  mvn reg_cycles, r0                      /* load new cycle count          */;\
                                                                             ;\
  ldr r0, [reg_base, #CHANGED_PC_STATUS]  /* load PC changed status        */;\
  cmp r0, #0                              /* see if PC has changed         */;\
  beq 1f                                  /* if not return                 */;\
  mov r1, #0;\
  str r1, [reg_base, #CHANGED_PC_STATUS];\
                                                                             ;\
  ldr r0, [reg_base, #REG_PC]             /* load new PC                   */;\
  ldr r1, [reg_base, #REG_CPSR]           /* r1 = flags                    */;\
  tst r1, #0x20                           /* see if Thumb bit is set       */;\
  bne 2f                                  /* if so load Thumb PC           */;\
                                                                             ;\
  load_registers_arm()                    /* load ARM regs                 */;\
  call_c_function(block_lookup_address_arm)                                  ;\
  restore_flags()                                                            ;\
  bx r0                                   /* jump to new ARM block         */;\
                                                                             ;\
1:                                                                           ;\
  load_registers_##mode()                 /* reload registers              */;\
  restore_flags()                                                            ;\
  return_##return_op()                                                       ;\
                                                                             ;\
2:                                                                           ;\
  load_registers_thumb()                  /* load Thumb regs               */;\
  call_c_function(block_lookup_address_thumb)                                ;\
  restore_flags()                                                            ;\
  bx r0                                   /* jump to new ARM block         */;\


arm_update_gba_builder(arm, arm, straight)

arm_update_gba_builder(thumb, thumb, straight)

arm_update_gba_builder(idle_arm, arm, add)
arm_update_gba_builder(idle_thumb, thumb, add)

arm_update_gba_to_arm:
  load_pc_straight()
  str r0, [reg_base, #REG_PC]             /* write out the PC              */

  save_flags()
  collapse_flags(r0)                      /* update the flags              */

  store_registers_thumb()                /* save out registers            */
  mov r0, reg_cycles
  call_c_function(NDS_update_dynarec)    /*          update GBA state              */

  mvn reg_cycles, r0                      /* load new cycle count          */

  ldr r0, [reg_base, #REG_PC]             /* load PC                   */
  ldr r1, [reg_base, #REG_CPSR]           /* r1 = flags                    */
  bic r1, r1, #0x20 /*sets to ARM mode*/

  load_registers_arm()                  /* load ARM regs               */
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  bic r1, r1, #0x20                       @ clear Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   /* jump to new ARM block         */

arm_update_gba_idle_to_arm:
  load_pc_add()
  str r0, [reg_base, #REG_PC]             /* write out the PC              */

  save_flags()
  collapse_flags(r0)                      /* update the flags              */

  store_registers_thumb()                /* save out registers            */
  mov r0, reg_cycles
  call_c_function(NDS_update_dynarec)    /*          update GBA state              */

  mvn reg_cycles, r0                      /* load new cycle count          */

  ldr r0, [reg_base, #REG_PC]             /* load PC                   */
  ldr r1, [reg_base, #REG_CPSR]           /* r1 = flags                    */
  bic r1, r1, #0x20 /*sets to ARM mode*/

  load_registers_arm()                  /* load ARM regs               */
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  bic r1, r1, #0x20                       @ clear Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   /* jump to new ARM block         */

arm_update_gba_to_thumb:
  load_pc_straight()
  str r0, [reg_base, #REG_PC]             /* write out the PC              */

  save_flags()
  collapse_flags(r0)                      /* update the flags              */

  store_registers_arm()                /* save out registers            */
  mov r0, reg_cycles
  call_c_function(NDS_update_dynarec)    /*          update GBA state              */

  mvn reg_cycles, r0                      /* load new cycle count          */

ldr r0, [reg_base, #REG_PC]             /* load PC                   */
  load_registers_thumb()                  /* load ARM regs               */
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  orr r1, r1, #0x20                       @ set Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0                                   /* jump to new thumb block         */

arm_update_gba_idle_to_thumb:
  load_pc_add()
  str r0, [reg_base, #REG_PC]             /* write out the PC              */

  save_flags()
  collapse_flags(r0)                      /* update the flags              */

  store_registers_arm()                /* save out registers            */
  mov r0, reg_cycles
  call_c_function(NDS_update_dynarec)    /*          update GBA state              */

  mvn reg_cycles, r0                      /* load new cycle count          */
ldr r0, [reg_base, #REG_PC]             /* load PC                   */
  load_registers_thumb()                  /* load ARM regs               */
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  orr r1, r1, #0x20                       @ set Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0                                   /* jump to new thumb block         */

@ These are b stubs for performing indirect branches. They are not
@ linked to and don't return, instead they link elsewhere.

@ Input:
@ r0: PC to branch to

arm_indirect_branch_arm:
  save_flags()
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0

arm_indirect_branch_thumb:
  save_flags()
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0

arm_indirect_branch_dual_arm:
  save_flags()
  tst r0, #0x01                           @ check lower bit
  bne 1f                                  @ if set going to Thumb mode
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   @ return

1:
  bic r0, r0, #0x01
  store_registers_arm()                   @ save out ARM registers
  load_registers_thumb()                  @ load in Thumb registers
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  orr r1, r1, #0x20                       @ set Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0                                   @ return

arm_indirect_branch_dual_thumb:
  save_flags()
  tst r0, #0x01                           @ check lower bit
  beq 1f                                  @ if set going to ARM mode
  bic r0, r0, #0x01
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0                                   @ return

1:
  store_registers_thumb()                 @ save out Thumb registers
  load_registers_arm()                    @ load in ARM registers
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  bic r1, r1, #0x20                       @ clear Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   @ return

arm_indirect_branch_to_arm:
  save_flags()
  store_registers_thumb()                 @ save out Thumb registers
  load_registers_arm()                    @ load in ARM registers
  ldr r1, [reg_base, #REG_CPSR]           @ load cpsr
  bic r1, r1, #0x20                       @ clear Thumb mode
  str r1, [reg_base, #REG_CPSR]           @ store flags
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   @ return


@ Update the cpsr.

@ Input:
@ r0: new cpsr value
@ r1: bitmask of which bits in cpsr to update
@ r2: current PC

execute_store_cpsr:
  save_flags()
  and reg_flags, r0, r1                   @ reg_flags = new_cpsr & store_mask
  ldr r0, [reg_base, #REG_CPSR]           @ r0 = cpsr
  bic r0, r0, r1                          @ r0 = cpsr & ~store_mask
  orr reg_flags, reg_flags, r0            @ reg_flags = new_cpsr | cpsr

  mov r0, reg_flags                       @ also put new cpsr in r0

  store_registers_arm()                   @ save ARM registers
  ldr r2, [lr]                            @ r2 = pc
  call_c_function(execute_store_cpsr_body)
  load_registers_arm()                    @ restore ARM registers

  cmp r0, #0                              @ check new PC
  beq 1f                                  @ if it's zero, return

  call_c_function(block_lookup_address_arm)

  restore_flags()
  bx r0                                   @ return to new ARM address

1:
  restore_flags()
  add pc, lr, #4                          @ return


@ Update the current spsr.

@ Input:
@ r0: new cpsr value
@ r1: bitmask of which bits in spsr to update

execute_store_spsr:
  ldr r1, =spsr                           @ r1 = spsr
  ldr r2, [reg_base, #CPU_MODE]           @ r2 = CPU_MODE
  str r0, [r1, r2, lsl #2]                @ spsr[CPU_MODE] = new_spsr
  bx lr

@ Read the current spsr.

@ Output:
@ r0: spsr

execute_read_spsr:
  ldr r0, =spsr                           @ r0 = spsr
  ldr r1, [reg_base, #CPU_MODE]           @ r1 = CPU_MODE
  ldr r0, [r0, r1, lsl #2]                @ r0 = spsr[CPU_MODE]
  bx lr                                   @ return


@ Restore the cpsr from the mode spsr and mode shift.

@ Input:
@ r0: current pc

execute_spsr_restore:
  save_flags()
  ldr r1, =spsr                           @ r1 = spsr
  ldr r2, [reg_base, #CPU_MODE]           @ r2 = cpu_mode
  ldr r1, [r1, r2, lsl #2]                @ r1 = spsr[cpu_mode] (new cpsr)
  str r1, [reg_base, #REG_CPSR]           @ update cpsr
  mov reg_flags, r1                       @ also, update shadow flags

  @ This function call will pass r0 (address) and return it.
  store_registers_arm()                   @ save ARM registers
  call_c_function(execute_spsr_restore_body)

  ldr r1, [reg_base, #REG_CPSR]           @ r1 = cpsr
  tst r1, #0x20                           @ see if Thumb mode is set
  bne 2f                                  @ if so handle it

  load_registers_arm()                    @ restore ARM registers
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0

2:
  load_registers_thumb()                  @ load Thumb registers
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0



@ Setup the mode transition work for calling an SWI.

@ Input:
@ r0: current pc

#define execute_swi_builder(mode)                                            ;\
                                                                             ;\
execute_swi_##mode:                                                          ;\
ldr r0, [lr] ;\
  save_flags()                                                               ;\
  store_registers_##mode()                                                   ;\
  call_c_function(mode##_swi_exec)                        ;\
  load_registers_##mode()                                                    ;\
  restore_flags()                                                            ;\
 add pc, lr, #4                                                                     ;\

execute_swi_builder(arm)


execute_swi_thumb:
ldr r0, [lr]
  save_flags()
  store_registers_thumb()
  call_c_function(thumb_swi_exec)
  load_registers_thumb()
  restore_flags()
 add pc, lr, #4


//execute_swi_builder(thumb)


@ Wrapper for calling SWI functions in C (or can implement some in ASM if
@ desired)

#define execute_swi_function_builder(swi_function, mode)                     ;\
                                                                             ;\
  .global execute_swi_hle_##swi_function##_##mode                            ;\
execute_swi_hle_##swi_function##_##mode:                                     ;\
  save_flags()                                                               ;\
  store_registers_##mode()                                                   ;\
  call_c_function(execute_swi_hle_##swi_function##_c)                        ;\
  load_registers_##mode()                                                    ;\
  restore_flags()                                                            ;\
  bx lr                                                                      ;\

execute_swi_function_builder(div, arm)
execute_swi_function_builder(div, thumb)


@ Start program execution. Normally the mode should be Thumb and the
@ PC should be 0x8000000, however if a save state is preloaded this
@ will be different.

@ Input:
@ r0: initial value for cycle counter

@ Uses sp as reg_base; must hold consistently true.

execute_arm_translate:
  sub sp, sp, #0x100                      @ allocate room for register data

  mvn reg_cycles, r0                      @ load cycle counter

  mov r0, reg_base                        @ load reg_base into first param
  call_c_function(move_reg)               @ make reg_base the new reg ptr
  call_c_function(update_dynarec)

  sub sp, sp, #REG_BASE_OFFSET            @ allocate room for ptr table
  //bl load_ptr_read_function_table         @ load read function ptr table

  ldr r0, [reg_base, #REG_PC]             @ r0 = current pc
  ldr r1, [reg_base, #REG_CPSR]           @ r1 = flags
  tst r1, #0x20                           @ see if Thumb bit is set

  bne 1f                                  @ if so lookup thumb

  load_registers_arm()                    @ load ARM registers
  call_c_function(block_lookup_address_arm)
  extract_flags()                         @ load flags
  bx r0                                 @ jump to first ARM block
1:
  load_registers_thumb()                  @ load Thumb registers
  call_c_function(block_lookup_address_thumb)
  extract_flags()                         @ load flags
  bx r0                                   @ jump to first Thumb block
@ Write out to memory.

@ Input:
@ r0: address
@ r1: value
@ r2: current pc

#define execute_store_body(store_type, store_op)                             ;\
  save_flags()                                                               ;\
  stmdb sp!, { lr }                       /* save lr                       */;\
  tst r0, #0xF0000000                     /* make sure address is in range */;\
  bne ext_store_u##store_type             /* if not do ext store           */;\
                                                                             ;\
  ldr r2, =memory_map_write               /* r2 = memory_map_write         */;\
  mov lr, r0, lsr #15                     /* lr = page index of address    */;\
  ldr r2, [r2, lr, lsl #2]                /* r2 = memory page              */;\
                                                                             ;\
  cmp r2, #0                              /* see if map is ext             */;\
  beq ext_store_u##store_type             /* if so do ext store            */;\
                                                                             ;\
  mov r0, r0, lsl #17                     /* isolate bottom 15 bits in top */;\
  mov r0, r0, lsr #17                     /* like performing and 0x7FFF    */;\
  store_op r1, [r2, r0]                   /* store result                  */;\


#define store_align_8()                                                      ;\
  and r1, r1, #0xff                                                          ;\

#define store_align_16()                                                     ;\
  bic r0, r0, #0x01                                                          ;\
  extract_u16(r1, r1)                                                        ;\

#define store_align_32()                                                     ;\
  bic r0, r0, #0x03                                                          ;\


#define execute_store_builder(store_type, store_op, load_op)                 ;\
                                                                             ;\
execute_store_u##store_type:                                                 ;\
  save_flags()                                                               ;\
  call_c_function(ds_write##store_type)                                  ;\
  restore_flags();\
  add pc, lr, #4                          /* return*/ ;\
ext_store_u##store_type:                                                     ;\
  call_c_function(ds_write##store_type)                                  ;\
  restore_flags();\
  add pc, lr, #4                          /* return*/ ;\

execute_store_builder(8, strb, ldrb)
execute_store_builder(16, strh, ldrh)
//execute_store_builder(32, str, ldr)

execute_store_u32:
  save_flags()
  call_c_function(ds_write32)
  restore_flags()
  add pc, lr, #4                          /* return*/

execute_store_u32_safe:
  save_flags()
  call_c_function(ds_write32)
  restore_flags()
  bx lr                                    @ Return

ext_store_u32_safe:
  ldmia sp!, { lr }                       @ Restore lr
  call_c_function(ds_write32)         @ Perform 32bit store
  restore_flags()
  bx lr                                   @ Return

smc_write:
  call_c_function(flush_translation_cache_ram)

lookup_pc:
  ldr r0, [reg_base, #REG_PC]             @ r0 = new pc
  ldr r1, [reg_base, #REG_CPSR]           @ r1 = flags
  tst r1, #0x20                           @ see if Thumb bit is set
  beq lookup_pc_arm                       @ if not lookup ARM

lookup_pc_thumb:
  call_c_function(block_lookup_address_thumb)
  restore_flags()
  bx r0                                   @ jump to new Thumb block

lookup_pc_arm:
  call_c_function(block_lookup_address_arm)
  restore_flags()
  bx r0                                   @ jump to new ARM block


#define sign_extend_u8(reg)
#define sign_extend_u16(reg)
#define sign_extend_u32(reg)

#define sign_extend_s8(reg)                                                  ;\
  mov reg, reg, lsl #24                   /* shift reg into upper 8bits    */;\
  mov reg, reg, asr #24                   /* shift down, sign extending    */;\

#define sign_extend_s16(reg)                                                 ;\
  mov reg, reg, lsl #16                   /* shift reg into upper 16bits   */;\
  mov reg, reg, asr #16                   /* shift down, sign extending    */;\

#define execute_load_op_u8(load_op)                                          ;\
  mov r0, r0, lsl #17                                                        ;\
  load_op r0, [r2, r0, lsr #17]                                              ;\

#define execute_load_op_s8(load_op)                                          ;\
  mov r0, r0, lsl #17                                                        ;\
  mov r0, r0, lsr #17                                                        ;\
  load_op r0, [r2, r0]                                                       ;\

#define execute_load_op_u16(load_op)                                         ;\
  execute_load_op_s8(load_op)                                                ;\

#define execute_load_op_s16(load_op)                                         ;\
  execute_load_op_s8(load_op)                                                ;\

#define execute_load_op_u16(load_op)                                         ;\
  execute_load_op_s8(load_op)                                                ;\

#define execute_load_op_u32(load_op)                                         ;\
  execute_load_op_u8(load_op)                                                ;\


#define execute_load_builder(load_type, load_function, load_op, mask)        ;\
                                                                             ;\
execute_load_##load_type:                                                    ;\
  save_flags()                                                               ;\
  call_c_function(ds_read##load_function)    /*CHANGE*/                            ;\
  sign_extend_##load_type(r0)             /* sign extend result            */;\
  restore_flags()                                                            ;\
  add pc, lr, #4                          /* return                        */;\


execute_load_builder(u8, 8, ldrneb, #0xF0000000)
execute_load_builder(s8, 8, ldrnesb, #0xF0000000)
//execute_load_builder(u16, 16, ldrneh, #0xF0000001)


execute_load_builder(s16, 16, ldrnesh, #0xF0000001)
execute_load_u16:
  save_flags()
  call_c_function(ds_read16)    /*CHANGE*/
  sign_extend_u16(r0)             /* sign extend result            */
  restore_flags()
  add pc, lr, #4                          /* return                        */


//execute_load_builder(u32, 32, ldrne, #0xF0000000)

execute_load_u32:
  save_flags()
  call_c_function(ds_read32)    /*CHANGE*/
  sign_extend_u32(r0)             /* sign extend result            */
  restore_flags()
  add pc, lr, #4                          /* return                        */


arm_mcr_asm:
  ldr r0, [lr]
  save_flags()
  store_registers_arm()
  call_c_function(arm_mcr)
  load_registers_arm()
  restore_flags()
  add pc, lr, #4

arm_mrc_asm:
  ldr r0, [lr]
  save_flags()
  store_registers_arm()
  call_c_function(arm_mrc)
  load_registers_arm()
  restore_flags()
  add pc, lr, #4

switch_user:
  save_flags()
  store_registers_arm()
  call_c_function(switch_user_arm)
  load_registers_arm()
  restore_flags()
  bx lr

switch_back:
save_flags()
  store_registers_arm()
  call_c_function(switch_back_arm)
  load_registers_arm()
  restore_flags()
  bx lr

#define save_reg_scratch(reg)                                                 ;\
  ldr r2, [reg_base, #(REG_BASE_OFFSET + (reg * 4))]                          ;\
  str r2, [reg_base, #(REG_BASE_OFFSET + (reg * 4) + 128)]                    ;\

#define restore_reg_scratch(reg)                                              ;\
  ldr r2, [reg_base, #(REG_BASE_OFFSET + (reg * 4) + 128)]                    ;\
  str r2, [reg_base, #(REG_BASE_OFFSET + (reg * 4))]                          ;\

#define scratch_regs_thumb(type)                                              ;\
  type##_reg_scratch(0)                                                       ;\
  type##_reg_scratch(1)                                                       ;\
  type##_reg_scratch(2)                                                       ;\
  type##_reg_scratch(3)                                                       ;\
  type##_reg_scratch(4)                                                       ;\
  type##_reg_scratch(5)                                                       ;\

#define scratch_regs_arm(type)                                                ;\
  type##_reg_scratch(0)                                                       ;\
  type##_reg_scratch(1)                                                       ;\
  type##_reg_scratch(6)                                                       ;\
  type##_reg_scratch(9)                                                       ;\
  type##_reg_scratch(12)                                                      ;\
  type##_reg_scratch(14)                                                      ;\


step_debug_arm:
  save_flags()
  collapse_flags(r0)

  ldr r0, [reg_base, #REG_CPSR]           @ r1 = cpsr
  tst r0, #0x20                           @ see if Thumb bit is set

  ldr r0, [lr]                            @ load PC
  mvn r1, reg_cycles                      @ load cycle counter

  beq 1f                                  @ if not goto ARM mode

  scratch_regs_thumb(save)

  store_registers_thumb()                 @ write back Thumb regs
  call_c_function(step_debug)             @ call debug step
  scratch_regs_thumb(restore)
  restore_flags()
  add pc, lr, #4                          @ return

1:
  scratch_regs_arm(save)
  store_registers_arm()                   @ write back ARM regs
  call_c_function(step_debug)             @ call debug step
  scratch_regs_arm(restore)
  restore_flags()
  add pc, lr, #4                          @ return, skipping PC

.pool

.comm memory_map_read 0x8000
.comm memory_map_write 0x8000

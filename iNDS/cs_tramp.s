.align 4
.globl __tramp_begin
.globl __tramp_end
__tramp_begin:
#ifdef __arm64__
ldr x8, __tramp_tgt
br x8
#elif defined __arm__
.thumb
ldr r12, __tramp_tgt
bx r12
#elif defined __x86_64__
jmp *(%rip)
#endif
__tramp_tgt:
#ifdef __LP64__
.quad 0x4141414141414141
#else
.long 0x41414141
#endif
__tramp_end:

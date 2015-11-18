/*  Android NDK doesn't have its own ucontext.h */
#ifndef ucontext_h_seen
#define ucontext_h_seen

#include <asm/sigcontext.h>       /* for sigcontext */
#include <asm/signal.h>           /* for stack_t */

typedef struct ucontext {
	unsigned long uc_flags;
	struct ucontext *uc_link;
	stack_t uc_stack;
	struct sigcontext uc_mcontext;
	unsigned long uc_sigmask;
} ucontext_t;

#endif
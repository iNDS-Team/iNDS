/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer inline functions (PowerPC)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000, 2001, 2002, 2003, 2004, 2006 Free Software Foundation, Inc.
 * Written by Paolo Bonzini.
 *
 * This file is part of GNU lightning.
 *
 * GNU lightning is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU lightning is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with GNU lightning; see the file COPYING.LESSER; if not, write to the
 * Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 ***********************************************************************/



#ifndef __lightning_funcs_h
#define __lightning_funcs_h

#include <string.h>

// #if !defined(__GNUC__) && !defined(__GNUG__)
// #error Go get GNU C, I do not know how to flush the cache
// #error with this compiler.
// #else
// static void
// jit_flush_code(void *start, void *end)
// {
// #ifndef LIGHTNING_CROSS
  // register char *ddest, *idest;

  // static int cache_line_size;
  // if (cache_line_size == 0) {
    // char buffer[8192];
    // int i, probe;

    // /* Find out the size of a cache line by zeroing one */
    // memset(buffer, 0xFF, 8192);
    // __asm__ __volatile__ ("dcbz 0,%0" : : "r"(buffer + 4096));

    // /* Probe for the beginning of the cache line. */
    // for(i = 0, probe = 4096; probe; probe >>= 1)
      // if (buffer[i | probe] != 0x00)
        // i |= probe;

    // /* i is now just before the start of the cache line */
    // i++;
    // for(cache_line_size = 1; i + cache_line_size < 8192; cache_line_size <<= 1)
      // if (buffer[i + cache_line_size] != 0x00)
        // break;
  // }

  // /* Point end to the last byte being flushed.  */
  // end   =(void*)( (long)end - 1);

  // start =(void*)( (long)start - (((long) start) & (cache_line_size - 1)));
  // end   =(void*)( (long)end   - (((long) end) & (cache_line_size - 1)));

  // /* Force data cache write-backs */
  // for (ddest = (char *) start; ddest <= (char *) end; ddest += cache_line_size) {
    // __asm__ __volatile__ ("dcbst 0,%0" : : "r"(ddest));
  // }
  // __asm__ __volatile__ ("sync" : : );

  // /* Now invalidate the instruction cache */
  // for (idest = (char *) start; idest <= (char *) end; idest += cache_line_size) {
    // __asm__ __volatile__ ("icbi 0,%0" : : "r"(idest));
  // }
  // __asm__ __volatile__ ("isync" : : );
// #endif /* !LIGHTNING_CROSS */
// }

#define jit_get_cpu			jit_get_cpu
__jit_constructor static void
jit_get_cpu(void)
{
}
#endif /* __GNUC__ || __GNUG__ */

#endif /* __lightning_funcs_h */

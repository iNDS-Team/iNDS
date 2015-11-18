/*
Math-NEON:  Neon Optimised Math Library based on cmath
Contact:    lachlan.ts@gmail.com
Copyright (C) 2009  Lachlan Tychsen - Smith aka Adventus

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
Assumes the floating point value |x| < 2,147,483,648
*/

#include "math_neon.h"

float modf_c(float x, int *i)
{
	int n;
	n = (int)x;
	*i = n;
	x = x - (float)n;
	return x;
}


float modf_neon_hfp(float x, int *i)
{
#ifdef __MATH_NEON
	asm volatile (	
	"vcvt.s32.f32	d1, d0					\n\t"	//d1 = (int) d0; 
	"vcvt.f32.s32	d2, d1					\n\t"	//d2 = (float) d1;
	"vsub.f32		d0, d0, d2				\n\t"	//d0 = d0 - d2; 
	"vstr.i32		s2, [r0]				\n\t"	//[r0] = d1[0] 
	::: "d0", "d1", "d2"
	);		
#endif
}


float modf_neon_sfp(float x, int *i)
{
#ifdef __MATH_NEON
	asm volatile (
	"vdup.f32 		d0, r0					\n\t"	//d0 = {x, x}	
	"vcvt.s32.f32	d1, d0					\n\t"	//d1 = (int) d0; 
	"vcvt.f32.s32	d2, d1					\n\t"	//d2 = (float) d1;
	"vsub.f32		d0, d0, d2				\n\t"	//d0 = d0 - d2; 
	"vstr.i32		s2, [r1]				\n\t"	//[r0] = d1[0] 
	"vmov.f32 		r0, s0					\n\t"	//r0 = d0[0];
	::: "d0", "d1", "d2"
	);
		
#else
	return modf_c(x, i);
#endif
}

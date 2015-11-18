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

#include "math.h"
#include "math_neon.h"

float ldexpf_c(float m, int e)
{
	union {
		float 	f;
		int 	i;
	} r;
	r.f = m;
	r.i += (e << 23);
	return r.f;
}

float ldexpf_neon_hfp(float m, int e)
{
#ifdef __MATH_NEON
	float r;
	asm volatile (
	"lsl 			r0, r0, #23				\n\t"	//r0 = r0 << 23	
	"vdup.i32 		d1, r0					\n\t"	//d1 = {r0, r0}
	"vadd.i32 		d0, d0, d1				\n\t"	//d0 = d0 + d1
	::: "d0", "d1"
	);
#endif
}

float ldexpf_neon_sfp(float m, int e)
{
#ifdef __MATH_NEON
	float r;
	asm volatile (
	"lsl 			r1, r1, #23				\n\t"	//r1 = r1 << 23	
	"vdup.f32 		d0, r0					\n\t"	//d0 = {r0, r0}	
	"vdup.i32 		d1, r1					\n\t"	//d1 = {r1, r1}
	"vadd.i32 		d0, d0, d1				\n\t"	//d0 = d0 + d1
	"vmov.f32 		r0, s0					\n\t"	//r0 = s0
	::: "d0", "d1"
	);
#else
	return ldexpf_c(m,e);
#endif
}
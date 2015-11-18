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

/*
Test func : acosf(x)
Test Range: -1.0 < x < 1.0
Peak Error:	~0.005%
RMS  Error: ~0.001%
*/

const float __acosf_pi_2 = M_PI_2;

float acosf_c(float x)
{
	return __acosf_pi_2 - asinf_c(x);
}


float acosf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asinf_neon_hfp(x);
	asm volatile (
	"vdup.f32	 	d1, %0					\n\t"	//d1 = {pi/2, pi/2};
	"vsub.f32	 	d0, d1, d0				\n\t"	//d0 = d1 - d0;
	::"r"(__acosf_pi_2):
	);
#endif
}

float acosf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	acosf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return acosf_c(x);
#endif
}




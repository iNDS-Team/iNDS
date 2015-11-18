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

const float __sinhf_rng[2] = {
	1.442695041f,
	0.693147180f
};

const float __sinhf_lut[16] = {
	0.00019578093328483123,	//p7
	0.00019578093328483123,	//p7
	0.0014122663401803872, 	//p6
	0.0014122663401803872, 	//p6
	0.008336936973260111, 	//p5
	0.008336936973260111, 	//p5
	0.04165989275009526, 	//p4
	0.04165989275009526, 	//p4
	0.16666570253074878, 	//p3
	0.16666570253074878, 	//p3
	0.5000006143673624, 	//p2
	0.5000006143673624, 	//p2
	1.000000059694879, 		//p1
	1.000000059694879, 		//p1
	0.9999999916728642,		//p0
	0.9999999916728642		//p0
};


float sinhf_c(float x)
{
	float a, b, xx;
	xx = -x;
	a = expf_c(x);
	b = expf_c(xx);
	a = a - b;
	a = a * 0.5f;
	return a;
}


float sinhf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (
	"vdup.f32 		d0, d0[0]				\n\t"	//d0 = {x, x}	
	"fnegs 			s1, s1					\n\t"	//s1 = -s1
	
	//Range Reduction:
	"vld1.32 		d2, [%0]				\n\t"	//d2 = {invrange, range}
	"vld1.32 		{d16, d17}, [%1]!		\n\t"	
	"vmul.f32 		d6, d0, d2[0]			\n\t"	//d6 = d0 * d2[0] 
	"vcvt.s32.f32 	d6, d6					\n\t"	//d6 = (int) d6
	"vld1.32 		{d18}, [%1]!			\n\t"	
	"vcvt.f32.s32 	d1, d6					\n\t"	//d1 = (float) d6
	"vld1.32 		{d19}, [%1]!			\n\t"	
	"vmls.f32 		d0, d1, d2[1]			\n\t"	//d0 = d0 - d1 * d2[1]
	"vld1.32 		{d20}, [%1]!			\n\t"	
		
	//polynomial:
	"vmla.f32 		d17, d16, d0			\n\t"	//d17 = d17 + d16 * d0;	
	"vld1.32 		{d21}, [%1]!			\n\t"	
	"vmla.f32 		d18, d17, d0			\n\t"	//d18 = d18 + d17 * d0;	
	"vld1.32 		{d22}, [%1]!			\n\t"	
	"vmla.f32 		d19, d18, d0			\n\t"	//d19 = d19 + d18 * d0;	
	"vld1.32 		{d23}, [%1]!			\n\t"	
	"vmla.f32 		d20, d19, d0			\n\t"	//d20 = d20 + d19 * d0;	
	"vmla.f32 		d21, d20, d0			\n\t"	//d21 = d21 + d20 * d0;	
	"vmla.f32 		d22, d21, d0			\n\t"	//d22 = d22 + d21 * d0;	
	"vmla.f32 		d23, d22, d0			\n\t"	//d23 = d23 + d22 * d0;	
	
	//multiply by 2 ^ m 	
	"vshl.i32 		d6, d6, #23				\n\t"	//d6 = d6 << 23		
	"vadd.i32 		d0, d23, d6				\n\t"	//d0 = d22 + d6		

	"vdup.f32 		d2, d0[1]				\n\t"	//d2 = s1		
	"vmov.f32 		d1, #0.5				\n\t"	//d1 = 0.5		
	"vsub.f32 		d0, d0, d2				\n\t"	//d0 = d0 - d2		
	"vmul.f32 		d0, d1					\n\t"	//d0 = d0 * d1		

	:: "r"(__sinhf_rng), "r"(__sinhf_lut) 
    : "d0", "d1", "q1", "q2", "d6"
	);
	
#endif
}

float sinhf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	sinhf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return sinhf_c(x);
#endif
};

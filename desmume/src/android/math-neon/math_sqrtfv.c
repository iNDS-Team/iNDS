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
Test func : sqrtf(x)
Test Range: 0 < x < 1,000,000,000
Peak Error:	~0.0010%
RMS  Error: ~0.0005%
*/

#include "math.h"
#include "math_neon.h"

void sqrtfv_c(float *x, int n, float *r)
{

	float x0, x1;
	float b0, b1, c0, c1;
	int m0, m1;
	union {
		float 	f;
		int 	i;
	} a0, a1;


	if (n & 0x1){
		*r++ = sqrtf_c(*x++);
		n--;
	}

	while(n > 0){
	
		x0 = *x++;
		x1 = *x++;
	
		//fast invsqrt approx
		a0.f = x0;
		a1.f = x1;
		a0.i = 0x5F3759DF - (a0.i >> 1);		//VRSQRTE
		a1.i = 0x5F3759DF - (a1.i >> 1);		//VRSQRTE
		c0 = x0 * a0.f;
		c1 = x1 * a1.f;
		b0 = (3.0f - c0 * a0.f) * 0.5;		//VRSQRTS
		b1 = (3.0f - c1 * a1.f) * 0.5;		//VRSQRTS
		a0.f = a0.f * b0;		
		a1.f = a1.f * b1;		
		c0 = x0 * a0.f;
		c1 = x1 * a1.f;
		b0 = (3.0f - c0 * a0.f) * 0.5;		//VRSQRTS
		b1 = (3.0f - c1 * a1.f) * 0.5;		//VRSQRTS
		a0.f = a0.f * b0;		
		a1.f = a1.f * b1;		

		//fast inverse approx
		c0 = a0.f;
		c0 = a1.f;
		m0 = 0x3F800000 - (a0.i & 0x7F800000);
		m1 = 0x3F800000 - (a1.i & 0x7F800000);
		a0.i = a0.i + m0;
		a1.i = a1.i + m1;
		a0.f = 1.41176471f - 0.47058824f * a0.f;
		a1.f = 1.41176471f - 0.47058824f * a1.f;
		a0.i = a0.i + m0;
		a1.i = a1.i + m1;
		b0 = 2.0 - a0.f * c0;
		b1 = 2.0 - a1.f * c1;
		a0.f = a0.f * b0;	
		a1.f = a1.f * b1;	
		b0 = 2.0 - a0.f * c0;
		b1 = 2.0 - a1.f * c1;
		a0.f = a0.f * b0;
		a1.f = a1.f * b1;
		
		*r++ = a0.f;
		*r++ = a1.f;
		n -= 2;

	}
}

void sqrtfv_neon(float *x, int n, float *r)
{
#ifdef __MATH_NEON
	asm volatile (

	"tst 			r1, #1 					\n\t"	//r1 & 1
	"beq 			1f 						\n\t"	//

	"vld1.32		d0[0], [r0]! 			\n\t"	//s0 = *x++
	"mov 			ip, lr 					\n\t"	//ip = lr
	//"bl 			sqrtf_neon_hfp 			\n\t"	//sqrtf_neon
	"mov 			lr, ip 					\n\t"	//lr = ip
	"vst1.32		d0[0], [r2]! 			\n\t"	//*r++ = r0
	"subs 			r1, r1, #1				\n\t"	//r1 = r1 - 1;		
	"bxeq 			lr						\n\t"	//

	"1:				 						\n\t"	//

	"vld1.32 		d0, [r0]! 				\n\t"	//d0 = (*x[0], *x[1]), x+=2;
	
	//fast invsqrt approx
	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d3 = d0 * d2
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d4 = (3 - d0 * d3) / 2 	
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4	
	"vmul.f32 		d2, d0, d1				\n\t"	//d3 = d0 * d2	
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d4 = (3 - d0 * d3) / 2	
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4	
		
	//fast reciporical approximation
	"vrecpe.f32		d1, d0					\n\t"	//d1 = ~ 1 / d0; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d1, d1, d2				\n\t"	//d1 = d1 * d2; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d0, d1, d2				\n\t"	//d0 = d1 * d2; 

	"vst1.64 		d0, [r2]!				\n\t"	//*r++ = d0;
	"subs 			r1, r1, #2				\n\t"	//n = n - 2; update flags
	"bgt 			1b 						\n\t"	//

	::: "d0", "d1", "d2", "d3"
);
#else
	sqrtfv_c(x, n, r);
#endif
}

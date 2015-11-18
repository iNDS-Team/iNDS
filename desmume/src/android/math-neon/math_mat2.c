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
Matrices are specified in column major format:

| a c |
| b d |

therefore m[2] = c
*/

#include "math_neon.h"

//matrix matrix multipication. d = m0 * m1;
void
matmul2_c(float m0[4], float m1[4], float d[4])
{	
	d[0] = m0[0]*m1[0] + m0[2]*m1[1];	
	d[1] = m0[1]*m1[0] + m0[3]*m1[1];
	d[2] = m0[0]*m1[2] + m0[2]*m1[3];
	d[3] = m0[1]*m1[2] + m0[3]*m1[3];
}

void
matmul2_neon(float m0[4], float m1[4], float d[4])
{	
#ifdef __MATH_NEON
	asm volatile (
	"vld1.32 		{d0, d1}, [%0]			\n\t"	//Q1 = m0
	"vld1.32 		{d2, d3}, [%1]			\n\t"	//Q2 = m1
	
	"vmul.f32 		d4, d0, d2[0]			\n\t"	//D4 = D0*D2[0]
	"vmul.f32 		d5, d0, d3[0]			\n\t"	//D5 = D0*D3[0]
	"vmla.f32 		d4, d1, d2[1]			\n\t"	//D4 += D1*D2[1]
	"vmla.f32 		d5, d1, d3[1]			\n\t"	//D5 += D1*D3[1]
	
	"vst1.32 		{d4, d5}, [%2] 			\n\t"	//Q4 = m+12	
	:: "r"(m0), "r"(m1), "r"(d) 
    : "q0", "q1", "q2", "memory"
	);	
#else
	matmul2_c(m0, m1, d);
#endif
}


//matrix vector multiplication. d = m * v
void
matvec2_c(float m[4], float v[2], float d[2])
{
	d[0] = m[0]*v[0] + m[2]*v[1];
	d[1] = m[1]*v[0] + m[3]*v[1];
}

void
matvec2_neon(float m[4], float v[2], float d[2])
{
#ifdef __MATH_NEON
	asm volatile (
	"vld1.32        d0, [%1]				\n\t"	//d0 = v
	"vld1.32 		{d1, d2}, [%0]			\n\t"	//Q1 = m
	
	"vmul.f32 		d3, d1, d0[0]			\n\t"	//Q5 = Q1*d0[0]
	"vmla.f32 		d3, d2, d0[1]			\n\t"	//Q5 += Q1*d0[1] 
	
	"vst1.32 		d3, [%2] 				\n\t"	//Q4 = m+12	
	:: "r"(m), "r"(v), "r"(d) 
    : "d0", "d1", "d2","d3", "memory"
	);	
#else
	matvec2_c(m, v, d);
#endif
}

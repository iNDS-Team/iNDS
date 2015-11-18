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


#include "math_neon.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#define randf()	(rand() / (RAND_MAX + 1.0f))

#ifdef ANDROID
#include "../main.h"
#include "../neontest.h"
#define printf LOGI
#endif

struct	test1_s {
	const char*	name;
	float 		(*func)(float);	//the function
	float 		(*bench)(float);	//the function to benchmark against.
	float 		rng0, rng1;
	int			num;
	float 		emaxabs;
	float 		xmaxabs;
	float 		emaxrel;
	float 		xmaxrel;
	float 		erms;
	int			time;				//time to execute num functions;
};

struct	test2_s {
	const char*	name;
	float 		(*func)(float, float);	//the function
	float 		(*bench)(float, float);	//the function to benchmark against.
	float 		rng0, rng1;
	int			num;
	float 		emaxabs;
	float 		xmaxabs;
	float 		emaxrel;
	float 		xmaxrel;
	float 		erms;
	int			time;				//time to execute num functions;
};


float invsqrtf(float x){
	return (1.0f / sqrtf(x));
}

typedef struct test1_s test1_t;
typedef struct test2_s test2_t;

test1_t test1[51] = 
{
	{"sinf       ", 	sinf, 		sinf, 	-M_PI, 		M_PI, 	500000},
	{"sinf_c     ", 	sinf_c, 	sinf, 	-M_PI, 		M_PI, 	500000},
	{"sinf_neon  ", 	sinf_neon, 	sinf, 	-M_PI, 		M_PI, 	500000},
	
	{"cosf       ", 	cosf, 		cosf, 	-M_PI, 		M_PI, 	500000},
	{"cosf_c     ", 	cosf_c, 	cosf, 	-M_PI, 		M_PI, 	500000},
	{"cosf_neon  ", 	cosf_neon, 	cosf, 	-M_PI, 		M_PI, 	500000},

	{"tanf       ", 	tanf, 		tanf, 	-M_PI_4, 	M_PI_4, 500000, 0, 0, 0},
	{"tanf_c     ", 	tanf_c, 	tanf, 	-M_PI_4, 	M_PI_4, 500000, 0, 0, 0},
	{"tanf_neon  ", 	tanf_neon, 	tanf, 	-M_PI_4, 	M_PI_4, 500000, 0, 0, 0},

	{"asinf      ", 	asinf, 		asinf, 	-1, 		1, 		500000, 0, 0, 0},
	{"asinf_c    ", 	asinf_c, 	asinf, 	-1, 		1,	 	500000, 0, 0, 0},
	{"asinf_neon ",		asinf_neon,	asinf, 	-1, 		1, 		500000, 0, 0, 0},
	
	{"acosf      ", 	acosf, 		acosf, 	-1, 		1, 		500000, 0, 0, 0},
	{"acosf_c    ", 	acosf_c, 	acosf, 	-1, 		1,	 	500000, 0, 0, 0},
	{"acosf_neon ",		acosf_neon,	acosf, 	-1, 		1, 		500000, 0, 0, 0},
	
	{"atanf      ", 	atanf, 		atanf, 	-1, 		1, 		500000, 0, 0, 0},
	{"atanf_c    ", 	atanf_c, 	atanf, 	-1, 		1,	 	500000, 0, 0, 0},
	{"atanf_neon ",		atanf_neon,	atanf, 	-1, 		1, 		500000, 0, 0, 0},

	{"sinhf       ", 	sinhf, 		sinhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"sinhf_c     ", 	sinhf_c, 	sinhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"sinhf_neon  ", 	sinhf_neon, sinhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	
	{"coshf       ", 	coshf, 		coshf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"coshf_c     ", 	coshf_c, 	coshf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"coshf_neon  ", 	coshf_neon, coshf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},

	{"tanhf       ", 	tanhf, 		tanhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"tanhf_c     ", 	tanhf_c, 	tanhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},
	{"tanhf_neon  ", 	tanhf_neon, tanhf, 	-M_PI, 		M_PI, 	500000, 0, 0, 0},

	{"expf       ", 	expf, 		expf, 	0, 			10, 	500000, 0, 0, 0},
	{"expf_c     ", 	expf_c, 	expf, 	0, 			10, 	500000, 0, 0, 0},
	{"expf_neon  ",		expf_neon, 	expf, 	0, 			10, 	500000, 0, 0, 0},
	
	{"logf       ", 	logf, 		logf, 	1, 			1000, 	500000, 0, 0, 0},
	{"logf_c     ", 	logf_c, 	logf, 	1, 			1000, 	500000, 0, 0, 0},
	{"logf_neon  ",		logf_neon, 	logf, 	1, 			1000, 	500000, 0, 0, 0},

	{"log10f       ", 	log10f, 	log10f, 1, 			1000, 	500000, 0, 0, 0},
	{"log10f_c     ", 	log10f_c, 	log10f, 1, 			1000, 	500000, 0, 0, 0},
	{"log10f_neon  ",	log10f_neon,log10f, 1, 			1000, 	500000, 0, 0, 0},

	{"floorf     ", 	floorf, 	floorf, 1, 			1000, 	5000000, 0, 0, 0},
	{"floorf_c   ", 	floorf_c, 	floorf, 1, 			1000, 	5000000, 0, 0, 0},
	{"floorf_neon",		floorf_neon,floorf, 1, 			1000, 	5000000, 0, 0, 0},

	{"ceilf     ", 		ceilf, 		ceilf, 	1, 			1000, 	5000000, 0, 0, 0},
	{"ceilf_c   ", 		ceilf_c, 	ceilf, 	1, 			1000, 	5000000, 0, 0, 0},
	{"ceilf_neon",		ceilf_neon,	ceilf, 	1, 			1000, 	5000000, 0, 0, 0},

	{"fabsf     ", 		fabsf, 		fabsf, 	1, 			1000, 	5000000, 0, 0, 0},
	{"fabsf_c   ", 		fabsf_c, 	fabsf, 	1, 			1000, 	5000000, 0, 0, 0},
	{"fabsf_neon",		fabsf_neon,	fabsf, 	1, 			1000, 	5000000, 0, 0, 0},

	{"sqrtf      ", 	sqrtf, 		sqrtf, 	1, 			1000, 	500000, 0, 0, 0},
	{"sqrtf_c    ", 	sqrtf_c, 	sqrtf, 	1, 			1000, 	500000, 0, 0, 0},
	{"sqrtf_neon ",		sqrtf_neon,	sqrtf, 	1, 			1000, 	500000, 0, 0, 0},

	{"invsqrtf      ", 	invsqrtf, 		invsqrtf, 	1, 	1000, 	500000, 0, 0, 0},
	{"invsqrtf_c    ", 	invsqrtf_c, 	invsqrtf, 	1, 	1000, 	500000, 0, 0, 0},
	{"invsqrtf_neon ",	invsqrtf_neon,	invsqrtf, 	1, 	1000, 	500000, 0, 0, 0},
};

test2_t test2[9] = 
{
	{"atan2f       ", 	atan2f, 	atan2f, 0.1, 		10, 	10000, 0, 0, 0},
	{"atan2f_c     ", 	atan2f_c, 	atan2f, 0.1, 		10, 	10000, 0, 0, 0},
	{"atan2f_neon  ", 	atan2f_neon,atan2f, 0.1, 		10, 	10000, 0, 0, 0},
	
	{"powf       ", 	powf, 		powf, 	1, 			10, 	10000, 0, 0, 0},
	{"powf_c     ", 	powf_c, 	powf, 	1, 			10, 	10000, 0, 0, 0},
	{"powf_neon  ", 	powf_neon, 	powf, 	1, 			10, 	10000, 0, 0, 0},

	{"fmodf       ", 	fmodf, 		fmodf, 	1, 			10, 	10000, 0, 0, 0},
	{"fmodf_c     ", 	fmodf_c, 	fmodf, 	1, 			10, 	10000, 0, 0, 0},
	{"fmodf_neon  ", 	fmodf_neon, fmodf, 	1, 			10, 	10000, 0, 0, 0},

};


void 
test_mathfunc1(test1_t *tst)
{

	float x;
	float dx = (tst->rng1 - tst->rng0) / ((float)tst->num);
#ifndef WIN32
	struct rusage ru;
#endif

	tst->emaxabs = tst->xmaxabs = 0;
	tst->emaxrel = tst->xmaxrel = 0;
	tst->erms = 0;
	for(x = tst->rng0; x < tst->rng1 ; x += dx){	
		float r = (tst->func)((float)x);
		float rr = (tst->bench)((float)x);
		float dr = fabs(r - rr);
		float drr = dr * (100.0f / rr);
		tst->erms += dr*dr;
		if (dr > tst->emaxabs){
			tst->emaxabs = dr;
			tst->xmaxabs = x;
		}
		if (drr > tst->emaxrel){
			tst->emaxrel = drr;
			tst->xmaxrel = x;
		}
	}
	tst->erms = sqrt(tst->erms / ((float) tst->num));
	
#ifdef WIN32
	tst->time = (1000 * clock()) / (CLOCKS_PER_SEC / 1000);
#else
	getrusage(RUSAGE_SELF, &ru);	
	tst->time = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
#endif

	for(x = tst->rng0; x < tst->rng1 ; x += dx){	
		(tst->func)((float)x);
	}

#ifdef WIN32
	tst->time = (1000 * clock()) / (CLOCKS_PER_SEC / 1000) - tst->time;
#else
	getrusage(RUSAGE_SELF, &ru);	
	tst->time = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec - tst->time;
#endif

}

void
test_mathfunc2(test2_t *tst)
{
	float x, y;
	float rng = tst->rng1 - tst->rng0;
	float d = (rng * rng) / ((float) tst->num);
#ifndef WIN32
	struct rusage ru;
#endif

	tst->emaxabs = tst->xmaxabs = 0;
	tst->emaxrel = tst->xmaxrel = 0;
	for(y = (tst->rng0); y < (tst->rng1) ; y += d){	
		for(x = (tst->rng0); x < (tst->rng1); x += d){	
			float r = (tst->func)((float)x, y);
			float rr = (tst->bench)((float)x, y);
			float dr = fabs(r - rr);
			float drr = dr * (100.0f / rr);
			if (dr > tst->emaxabs){
				tst->emaxabs = dr;
				tst->xmaxabs = x;
			}
			if (drr > tst->emaxrel && fabsf(rr) > 0.0001){
				tst->emaxrel = drr;
				tst->xmaxrel = x;
			}
		}
	}
	
#ifdef WIN32
	tst->time = (1000 * clock()) / (CLOCKS_PER_SEC / 1000) ;
#else
	getrusage(RUSAGE_SELF, &ru);	
	tst->time = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
#endif

	for(y = tst->rng0; y < tst->rng1 ; y += d){	
		for(x = tst->rng0; x < tst->rng1 ; x += d){	
			(tst->func)((float)x, (float)y);
		}
	}

#ifdef WIN32
	tst->time = (1000 * clock()) / (CLOCKS_PER_SEC / 1000) - tst->time;
#else
	getrusage(RUSAGE_SELF, &ru);	
	tst->time = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec - tst->time;
#endif

}

void test_vectorfunc()
{
	float v0[4], v1[4], d[4];
	
	for(int i=0;i<4;i++)
	{
		v0[i] = 10*randf() - 5;
		v1[i] = 10*randf() - 5;
		d[i] = 10*randf() - 5;		
	}
	
	int testnum = 5000000;
	struct rusage ru;
	int v2t[3], v3t[3], v4t[3];
	float r;
	
	printf("\n");
	
	//dot 2
	/*getrusage(RUSAGE_SELF, &ru);	
	v2t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot2_c(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v2t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot2_neon(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v2t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	r = dot2_c(v0, v1);
	printf("dot2_c = %f\n", r);
	r = dot2_neon(v0, v1);
	printf("dot2_neon = %f\n", r);
	
	printf("dot2: c=%i \t neon=%i \t rate=%.2f \n", v2t[1] - v2t[0], v2t[2] - v2t[1], 
	(float)(v2t[1] - v2t[0]) / (float)(v2t[2] - v2t[1]));*/

	//normalize 2
	getrusage(RUSAGE_SELF, &ru);	
	v2t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize2_c(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v2t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize2_neon(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v2t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;


	normalize2_c(v0, d);
	printf("normalize2_c = [%.2f, %.2f]\n", d[0], d[1]);
	normalize2_neon(v0, d);
	printf("normalize2_neon = [%.2f, %.2f]\n", d[0], d[1]);
	
	printf("normalize2: c=%i \t neon=%i \t rate=%.2f \n", v2t[1] - v2t[0], v2t[2] - v2t[1], 
	(float)(v2t[1] - v2t[0]) / (float)(v2t[2] - v2t[1]));
	printf("\n");

	
	//dot 3
	/*getrusage(RUSAGE_SELF, &ru);	
	v3t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot3_c(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot3_neon(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	r = dot3_c(v0, v1);
	printf("dot3_c = %f\n", r);
	r = dot3_neon(v0, v1);
	printf("dot3_neon = %f\n", r);
	
	printf("dot3: c=%i \t neon=%i \t rate=%.2f \n", v3t[1] - v3t[0], v3t[2] - v3t[1], 
	(float)(v3t[1] - v3t[0]) / (float)(v3t[2] - v3t[1]));*/

	//normalize 3
	getrusage(RUSAGE_SELF, &ru);	
	v3t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize3_c(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize3_neon(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;


	normalize3_c(v0, d);
	printf("normalize3_c = [%.2f, %.2f, %.2f]\n", d[0], d[1], d[2]);
	normalize3_neon(v0, d);
	printf("normalize3_neon = [%.2f, %.2f, %.2f]\n", d[0], d[1], d[2]);
	
	printf("normalize3: c=%i \t neon=%i \t rate=%.2f \n", v3t[1] - v3t[0], v3t[2] - v3t[1], 
	(float)(v3t[1] - v3t[0]) / (float)(v3t[2] - v3t[1]));

	//cross 3
	getrusage(RUSAGE_SELF, &ru);	
	v3t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		cross3_c(v0, v1, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		cross3_neon(v0, v1, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v3t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;


	cross3_c(v0, v1, d);
	printf("cross3_c = [%.2f, %.2f, %.2f]\n", d[0], d[1], d[2]);
	cross3_neon(v0, v1, d);
	printf("cross3_neon = [%.2f, %.2f, %.2f]\n", d[0], d[1], d[2]);
	
	printf("cross3: c=%i \t neon=%i \t rate=%.2f \n", v3t[1] - v3t[0], v3t[2] - v3t[1], 
	(float)(v3t[1] - v3t[0]) / (float)(v3t[2] - v3t[1]));
	printf("\n");


	//dot 4
	/*getrusage(RUSAGE_SELF, &ru);	
	v4t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot4_c(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v4t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		r = dot4_neon(v0, v1);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v4t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	r = dot4_c(v0, v1);
	printf("dot4_c = %f\n", r);
	r = dot4_neon(v0, v1);
	printf("dot4_neon = %f\n", r);
	
	printf("dot4: c=%i \t neon=%i \t rate=%.2f \n", v4t[1] - v4t[0], v4t[2] - v4t[1], 
	(float)(v4t[1] - v4t[0]) / (float)(v4t[2] - v4t[1]));*/
	
	//normalize 4
	getrusage(RUSAGE_SELF, &ru);	
	v4t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize4_c(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v4t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(int i=0;i < testnum; i++)
	{
		normalize4_neon(v0, d);
	};
	getrusage(RUSAGE_SELF, &ru);	
	v4t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;


	normalize4_c(v0, d);
	printf("normalize4_c = [%.2f, %.2f, %.2f, %.2f]\n", d[0], d[1], d[2], d[3]);
	normalize4_neon(v0, d);
	printf("normalize4_neon = [%.2f, %.2f, %.2f, %.2f]\n", d[0], d[1], d[2], d[3]);
	
	printf("normalize4: c=%i \t neon=%i \t rate=%.2f \n", v4t[1] - v4t[0], v4t[2] - v4t[1], 
	(float)(v4t[1] - v4t[0]) / (float)(v4t[2] - v4t[1]));
	printf("\n");


}



void test_matrixfunc()
{
	float m0[16], m1[16], m2[16];
	int m2t[3], m3t[3], m4t[3];
	
	int i;
	int testnum = 1000000;
	struct rusage ru;
	
	for(int i=0;i<16;i++)
	{
		m0[i] = 10.0f * randf() - 5.0f; 
		m1[i] = 10.0f * randf() - 5.0f; 
		m2[i] = 10.0f * randf() - 5.0f; 
	}


	//matmul2 
	getrusage(RUSAGE_SELF, &ru);	
	m2t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul2_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m2t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul2_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m2t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	matmul2_c(m0, m1, m2);	
	printf("matmul2_c = \n");
	printf("\t\t\t|%.2f, %.2f|\n", m2[0], m2[2]);
	printf("\t\t\t|%.2f, %.2f|\n", m2[1], m2[3]);

	matmul2_neon(m0, m1, m2);	
	printf("matmul2_neon = \n");
	printf("\t\t\t|%.2f, %.2f|\n", m2[0], m2[2]);
	printf("\t\t\t|%.2f, %.2f|\n", m2[1], m2[3]);
	
	printf("matmul2: c=%i \t neon=%i \t rate=%.2f \n", m2t[1] - m2t[0], m2t[2] - m2t[1], 
		(float)(m2t[1] - m2t[0]) / (float)(m2t[2] - m2t[1]));


	//matvec2 
	getrusage(RUSAGE_SELF, &ru);	
	m2t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec2_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m2t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec2_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m2t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	memset(m2, 0, 4*sizeof(float));
	matvec2_c(m0, m1, m2);	
	printf("matvec2_c = |%.2f, %.2f|\n", m2[0], m2[1]);
	
	memset(m2, 0, 4*sizeof(float));
	matvec2_neon(m0, m1, m2);	
	printf("matvec2_neon = |%.2f, %.2f|\n", m2[0], m2[1]);

	printf("matvec2: c=%i \t neon=%i \t rate=%.2f \n", m2t[1] - m2t[0], m2t[2] - m2t[1], 
		(float)(m2t[1] - m2t[0]) / (float)(m2t[2] - m2t[1]));

	//MAT3
	getrusage(RUSAGE_SELF, &ru);	
	m3t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul3_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m3t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul3_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m3t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	memset(m2, 0, 9*sizeof(float));
	matmul3_c(m0, m1, m2);	
	printf("matmul3_c =\n");
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[0], m2[3], m2[6]);
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[1], m2[4], m2[7]);
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[2], m2[5], m2[8]);
	
	memset(m2, 0, 9*sizeof(float));
	matmul3_neon(m0, m1, m2);	
	printf("matmul3_neon =\n");
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[0], m2[3], m2[6]);
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[1], m2[4], m2[7]);
	printf("\t\t\t|%.2f, %.2f, %.2f|\n", m2[2], m2[5], m2[8]);
	
	printf("matmul3: c=%i \t neon=%i \t rate=%.2f \n", m3t[1] - m3t[0], m3t[2] - m3t[1], 
		(float)(m3t[1] - m3t[0]) / (float)(m3t[2] - m3t[1]));

	//matvec3
	getrusage(RUSAGE_SELF, &ru);	
	m3t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec3_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m3t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec3_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m3t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	memset(m2, 0, 4*sizeof(float));
	matvec3_c(m0, m1, m2);	
	printf("matvec3_c = |%.2f, %.2f, %.2f|\n", m2[0], m2[1], m2[2]);

	memset(m2, 0, 4*sizeof(float));
	matvec3_neon(m0, m1, m2);	
	printf("matvec3_neon = |%.2f, %.2f, %.2f|\n", m2[0], m2[1], m2[2]);
	
	printf("matvec3: c=%i \t neon=%i \t rate=%.2f \n", m2t[1] - m2t[0], m2t[2] - m2t[1], 
		(float)(m2t[1] - m2t[0]) / (float)(m2t[2] - m2t[1]));

	//MAT4
	getrusage(RUSAGE_SELF, &ru);	
	m4t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul4_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m4t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matmul4_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m4t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	memset(m2, 0, 16*sizeof(float));
	matmul4_c(m0, m1, m2);	
	printf("matmul4_c =\n");
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[0], m2[4], m2[8], m2[12]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[1], m2[5], m2[9], m2[13]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[2], m2[6], m2[10], m2[14]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[3], m2[7], m2[11], m2[15]);
	
	memset(m2, 0, 16*sizeof(float));
	matmul4_neon(m0, m1, m2);	
	printf("matmul4_neon =\n");
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[0], m2[4], m2[8], m2[12]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[1], m2[5], m2[9], m2[13]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[2], m2[6], m2[10], m2[14]);
	printf("\t\t\t|%.2f, %.2f, %.2f, %.2f|\n", m2[3], m2[7], m2[11], m2[15]);
	
	printf("matmul4: c=%i \t neon=%i \t rate=%.2f \n", m4t[1] - m4t[0], m4t[2] - m4t[1], 
		(float)(m4t[1] - m4t[0]) / (float)(m4t[2] - m4t[1]));

	//matvec4
	getrusage(RUSAGE_SELF, &ru);	
	m4t[0] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec4_c(m0, m1, m2);	
	}
	getrusage(RUSAGE_SELF, &ru);	
	m4t[1] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;
	for(i = 0; i < testnum; i++){
		matvec4_neon(m0, m1, m2);
	}
	getrusage(RUSAGE_SELF, &ru);	
	m4t[2] = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec;

	memset(m2, 0, 4*sizeof(float));
	matvec4_c(m0, m1, m2);	
	printf("matvec4_c = |%.2f, %.2f, %.2f, %f|\n", m2[0], m2[1], m2[2], m2[3]);

	memset(m2, 0, 4*sizeof(float));
	matvec4_neon(m0, m1, m2);	
	printf("matvec4_neon = |%.2f, %.2f, %.2f, %f|\n", m2[0], m2[1], m2[2], m2[3]);
	
	printf("matvec4: c=%i \t neon=%i \t rate=%.2f \n", m2t[1] - m2t[0], m2t[2] - m2t[1], 
		(float)(m2t[1] - m2t[0]) / (float)(m2t[2] - m2t[1]));


}

int math_debug_main(int argc, char** argv)
{	

	int i, ii;
	if (argc > 1 && strcmp(argv[1], "-norunfast") == 0){
		printf("RUNFAST: Disabled \n");
	}else {
		printf("RUNFAST: Enabled \n");
		enable_runfast();
	}

	srand(time(NULL));

#if 1
	//test single argument functions:
	printf("------------------------------------------------------------------------------------------------------\n");	
	printf("MATRIX FUNCTION TESTS \n");	
	printf("------------------------------------------------------------------------------------------------------\n");	
	
	test_matrixfunc();
	test_vectorfunc();

	printf("------------------------------------------------------------------------------------------------------\n");	
	printf("CMATH FUNCTION TESTS \n");	
	printf("------------------------------------------------------------------------------------------------------\n");	
	printf("Function\tRange\t\tNumber\tABS Max Error\tREL Max Error\tRMS Error\tTime\tRate\n");	
	printf("------------------------------------------------------------------------------------------------------\n");	
	for(i = 0; i < 51; i++){
		test_mathfunc1(&test1[i]);	
		
		ii = i - (i % 3);
		printf("%s\t", test1[i].name);
		printf("[%.2f, %.2f]\t", test1[i].rng0, test1[i].rng1);
		printf("%i\t", test1[i].num);
		printf("%.2e\t", test1[i].emaxabs);
		printf("%.2e%%\t", test1[i].emaxrel);
		printf("%.2e\t", test1[i].erms);
		printf("%i\t", test1[i].time);
		printf("x%.2f\t", (float)test1[ii].time / test1[i].time);
		printf("\n");
	}
	for(i = 0; i < 9; i++){
		test_mathfunc2(&test2[i]);
	
		ii = i - (i % 3);
		
		printf("%s\t", test2[i].name);
		printf("[%.2f, %.2f]\t", test2[i].rng0, test2[i].rng1);
		printf("%i\t", test2[i].num);
		printf("%.2e\t", test2[i].emaxabs);
		printf("%.2e%%\t", test2[i].emaxrel);
		printf("%.2e\t", test2[i].erms);
		printf("%i\t", test2[i].time);
		printf("x%.2f\t", (float)test2[ii].time / test2[i].time);
		printf("\n");
	}
	
#else


	float x = 0;
	for(x = -M_PI_2; x < M_PI_2; x+= 0.01)
	{
		printf("x=%.2f\t in=%.2f\t c=%.2f\t neon=%.2f \n", x, sinhf(x), sinhf_c(x), sinhf_neon(x));
	}

#endif
	
	return 0;
} 

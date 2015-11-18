//
//  main.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "AppDelegate.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

#include <limits.h>	   /* for PAGESIZE */
#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

//#define USE_TEMP_JIT

#ifdef USE_TEMP_JIT

typedef int (*inc_t)(int a);
inc_t _inc = NULL;

int
main(void)
{
	uint32_t code[] = {
		0xe2800001, // add	r0, r0, #1
		0xe12fff1e, // bx	lr
	};
	
	uint32_t *p;
	
	/* Allocate a buffer; it will have the default
	 protection of PROT_READ|PROT_WRITE. */
	p = malloc(1024+PAGESIZE-1);
	if (!p) {
		perror("Couldn't malloc(1024)");
		exit(errno);
	}
	
	/* Align to a multiple of PAGESIZE, assumed to be a power of two */
	p = (uint32_t *)(((int) p + PAGESIZE-1) & ~(PAGESIZE-1));
	
	// copy instructions to function
	p[0] = code[0];
	p[1] = code[1];
	
	/* Mark the buffer read / execute. */
	if (mprotect(p, 1024, PROT_READ | PROT_EXEC)) {
		perror("Couldn't mprotect");
		exit(errno);
	}
	
	_inc = (inc_t)p;
	int a = 1;
	a = _inc(a);
	printf("%d\n", a);	// as expected, echos 2
	
	exit(0);
}

#else

int main(int argc, char *argv[])
{
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif


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

#include <unistd.h> //getpagesize

//#define USE_TEMP_JIT

#ifdef USE_TEMP_JIT

typedef int (*inc_t)(int a);

int main(void)
{
    inc_t _inc = NULL;
    int PAGESIZE = getpagesize();
	uint32_t code[] = {
		0xd10043ff, // sub    sp, sp, #16
        0xb9000fe0, // str    w0, [sp, #12]
        0xb9400fe0, // ldr    w0, [sp, #12]
        0x11000400, // add    w0, w0, #1
        0x910043ff, // add    sp, sp, #16
        0xd65f03c0, // ret
	};
    
    
	
	uint32_t *p;
    
	/* Allocate a buffer; it will have the default
	 protection of PROT_READ|PROT_WRITE. */
	posix_memalign((void **)&p, PAGESIZE, 1024);
	if (!p) {
		perror("Couldn't malloc(1024)");
		exit(errno);
	}
	
	// copy instructions to function
    int size = sizeof(code);
    memcpy(p, code, size);
	
	/* Mark the buffer read / execute. */
    int result = mprotect(p, 1024, PROT_READ | PROT_EXEC);
	if (result == -1) {
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
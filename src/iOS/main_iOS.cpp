//
//  main.c
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#include "main_iOS.h"
#include <stdio.h>

#include "rasterize.h"

GPU3DInterface *core3DList[] = {
    &gpu3DNull,
    //&gpu3Dgl,
    &gpu3DRasterize,
    NULL
};

volatile bool execute = true;


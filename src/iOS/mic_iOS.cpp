//
//  mic_iOS.cpp
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 Will Cobb. All rights reserved.
//

#include <stdlib.h>
#include "mic.h"
#include "NDSSystem.h"
#include "readwrite.h"
#include "emufile.h"


BOOL Mic_Init(void)
{
    return true;
}

void Mic_DeInit(void)
{

}

void Mic_Reset(void)
{
    
}


u8 Mic_ReadSample(void)
{
    return 127;
}

void mic_savestate(EMUFILE* os)
{
    
}

bool mic_loadstate(EMUFILE* is, int size)
{
    return TRUE;
}


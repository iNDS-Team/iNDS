//
//  mic.h
//  iNDS
//
//  Created by Will Cobb on 4/5/2012.
//  Copyright (c) 2016 Homebrew. All rights reserved.
//

#ifndef _MIC_H_
#define _MIC_H_

#import <AudioToolbox/AudioToolbox.h>

#include <iostream>
#include "emufile.h"
#include "types.h"

BOOL Mic_Init(void);
void Mic_Reset(void);
void Mic_DeInit(void);
u8 Mic_ReadSample(void);

void mic_savestate(EMUFILE* os);
bool mic_loadstate(EMUFILE* is, int size);

#endif /* _MIC_H_ */


//
//  snd_iOS.h
//  iNDS
//
//  Created by Will Cobb on 1/6/2017.
//  Copyright (c) 2017 Will Cobb. All rights reserved.
//

#ifndef iNDS_sndcoreaudio_h
#define iNDS_sndcoreaudio_h

#include "SPU.h"

extern SoundInterface_struct SNDCoreAudio;

SoundInterface_struct *SNDCoreList[] = {
    &SNDDummy,
    //&SNDCoreAudio,
    NULL
};

#define SNDCORE_COREAUDIO 1


#endif

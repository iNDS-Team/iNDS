//
//  mic.cpp
//  iNDS
//
//  Created by Will Cobb on 4/5/2012.
//  Copyright (c) 2016 Homebrew. All rights reserved.
//

#include "mic.h"
#import "iNDSMicrophone.h"
#import "TPCircularBuffer.h"

// For easy timing functions... remove later
#import <UIKit/UIKit.h>

iNDSMicrophone *microphone;
TPCircularBuffer *buf;



void Mic_DeInit(){
    printf("Mic_DeInit\n");
}
BOOL Mic_Init(){
    printf("Mic Init\n");
    if (!microphone) {
        dispatch_async(dispatch_get_main_queue(), ^{
            microphone = [[iNDSMicrophone alloc] init];
            buf = microphone.buffer;
            [microphone start];
        });
    }
    return true;
}

void Mic_Reset(){
    printf("Mic_Reset\n");
    NSLog(@"%@", microphone);
    
}

// For debugging
void sampleRate() {
    static CFTimeInterval now = 0;
    static long count = 100000;
    if (count == 100000) {
        printf("Sample Rate: %f\n", 1 / ((CACurrentMediaTime() - now) / count));
        count = 0;
        now = CACurrentMediaTime();
    }
    count++;
}

// the closer the sample rate is to 16000, the better the microphone will work
u8 Mic_ReadSample(){
    if (!microphone)
        return 128;
#ifdef DEBUG
    sampleRate();
#endif
    int32_t availableBytes;
    u8 *stream = (u8 *)TPCircularBufferTail(buf, &availableBytes);
    int32_t index = availableBytes > 4096 ? 3082 : 0; // Skip when the buffer starts getting too big
    if (availableBytes > 0) {
        // The ds mic is much less sensitive so the noise needs to be dampened
        s8 sample = (stream[index] - 128);
        s8 neg = sample < 0 ? -1 : 1;
        sample = ((sample * sample) / 7) * neg;
        //
        TPCircularBufferConsume(buf, index + 1);
        printf("Sample: %d\n", sample + 128);
        return sample + 128;
    } else {
#ifdef DEBUG
        printf("No bytes to be read: %d (%p)\n", availableBytes, buf);
#endif
        [microphone start];
        buf = microphone.buffer;
        return 128;
    }
    
}

void mic_savestate(EMUFILE* os){
    printf("mic_savestate\n");
}

bool mic_loadstate(EMUFILE* is, int size){
    printf("mic_loadstate\n");
    return true;
}


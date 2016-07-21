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
#import <AVFoundation/AVFAudio.h>

// For easy timing functions... remove later
#import <UIKit/UIKit.h>

iNDSMicrophone *microphone;
TPCircularBuffer *buf;
bool micEnabled;
volatile float micSampleRate = 0.0;

#define SampleRateModifier 1

void Mic_DeInit(){
    printf("Mic_DeInit\n");
}
BOOL Mic_Init(){
    printf("Mic Init\n");
    if (!microphone && [[AVAudioSession sharedInstance] respondsToSelector:@selector(requestRecordPermission)]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            microphone = [[iNDSMicrophone alloc] init];
            micEnabled = microphone.micEnabled;
            buf = microphone.buffer;
            [microphone start];
        });
    }
    return true;
}

void Mic_Reset(){
    printf("Mic_Reset\n");
    micEnabled = microphone.micEnabled;
}

// For debugging
static inline void calcSampleRate() {
    static CFTimeInterval now = 0;
    static long count = 16000;
    if (count == 16000) {
        count = 0;
        now = CACurrentMediaTime();
        micSampleRate = 1 / ((CACurrentMediaTime() - now) / count);
    }
    count++;
}

// the closer the sample rate is to 16000, the better the microphone will work
u8 Mic_ReadSample(){
    if (!microphone || !micEnabled)
        return 128;

    calcSampleRate();
    
    int32_t availableBytes;
    u8 *stream = (u8 *)TPCircularBufferTail(buf, &availableBytes);
    int32_t index = availableBytes > 4096 ? (3081/SampleRateModifier) : 0; // Skip when the buffer starts overflowing
    if (availableBytes > 0) {
        TPCircularBufferConsume(buf, (index + 1) * SampleRateModifier);
        // The ds mic is much less sensitive so the sound needs to be dampened
        s8 sample = (stream[index] - 128);
        
        //printf("Sample: %d -> %d\n", (stream[index] - 128), sample/5);
        return sample / 5 + 128;
        
    } else {
//#ifdef DEBUG
//        printf("No bytes to be read: %d (%p)\n", availableBytes, buf);
//#endif
        micEnabled = microphone.micEnabled;
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
    micEnabled = microphone.micEnabled;
    return true;
}


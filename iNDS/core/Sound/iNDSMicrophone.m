//
//  iNDSMicrophone.m
//  iNDS
//
//  Created by Will Cobb on 4/6/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSMicrophone.h"
#import "EZAudio.h"
#import "AppDelegate.h"

TPCircularBuffer buffer;

@interface iNDSMicrophone () <EZMicrophoneDelegate> {
    EZMicrophone *microphone;
}

@end

@implementation iNDSMicrophone

- (id)init
{
    if (self = [super init]) {
        
        TPCircularBufferInit(&buffer, 1024 * 16);
        self.buffer = &buffer;
        
        AudioStreamBasicDescription audioDescription = {0};
        audioDescription.mFormatID          = kAudioFormatLinearPCM;
        audioDescription.mFormatFlags       = kAudioFormatFlagIsPacked;
        audioDescription.mChannelsPerFrame  = 1;
        audioDescription.mBytesPerPacket    = 1;
        audioDescription.mFramesPerPacket   = 1;
        audioDescription.mBytesPerFrame     = sizeof(UInt8);
        audioDescription.mBitsPerChannel    = 8 * sizeof(UInt8);
        audioDescription.mSampleRate        = 48000.0;
        
        microphone = [EZMicrophone microphoneWithDelegate:self withAudioStreamBasicDescription:audioDescription];
        
        //NSURL *url = [NSURL URLWithString:[AppDelegate.sharedInstance.documentsPath stringByAppendingPathComponent:@"test.m4a"]];
        //NSLog(@"Recording to: %@", url);
        //recorder = [EZRecorder recorderWithURL:url clientFormat:microphone.audioStreamBasicDescription fileType:EZRecorderFileTypeM4A];

        NSLog(@"STarting Mic");
    }
    return self;
}

- (BOOL)micEnabled
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"enableMic"];
}

- (void)start
{
    if (!microphone.microphoneOn) {
        TPCircularBufferClear(_buffer);
        microphone.microphoneOn = YES;
    }
    
}

- (void)stop
{
    microphone.microphoneOn = NO;
}

-(void)    microphone:(EZMicrophone *)microphone
        hasBufferList:(AudioBufferList *)bufferList
       withBufferSize:(UInt32)bufferSize
 withNumberOfChannels:(UInt32)numberOfChannels
{
    [self appendDataToCircularBuffer:&buffer fromAudioBufferList:bufferList];
    //[recorder appendDataFromBufferList:bufferList withBufferSize:bufferSize];
}

- (void)microphone:(EZMicrophone *)microphone changedPlayingState:(BOOL)isPlaying
{
    NSLog(@"Microphone changed playing state to: %d", isPlaying);
}

-(void)appendDataToCircularBuffer:(TPCircularBuffer*)circularBuffer
              fromAudioBufferList:(AudioBufferList*)audioBufferList {
    TPCircularBufferProduceBytes(circularBuffer, audioBufferList->mBuffers[0].mData, audioBufferList->mBuffers[0].mDataByteSize);
    
    // Stop microphone if it isn't being used
    int32_t availableBytes = _buffer->fillCount;
    if (availableBytes > 15000) {
        microphone.microphoneOn = NO;
        NSLog(@"Stopping Mic: %p", _buffer);
    }
}

- (void)dealloc
{
}

@end

//
//  iNDSEmulationController.m
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationController.h"
#import "iNDSEmulationView.h"
#import "iNDSGamePadView.h"
#import "iNDSROM.h"
#import "iNDSSettings.h"

#include "emu.h"
#include "types.h"
#include "render3D.h"
#include "rasterize.h"
#include "SPU.h"
#include "debug.h"
#include "NDSSystem.h"
#include "path.h"
#include "slot1.h"
#include "saves.h"
#include "cheatSystem.h"
#include "slot1.h"
#include "version.h"
#include "metaspu.h"

#define FRAME_TIME 0.01666 //60fps

@interface iNDSEmulationController () {
    struct NDS_fw_config_data fw_config;
    
    iNDSEmulationView   *_emulatorView;
    CGFloat timeOverlap;
    CGFloat _fps;
}

@property (nonatomic, strong) CADisplayLink *coreLink;
@property (nonatomic, weak) iNDSSettings    *settings;

@end

@implementation iNDSEmulationController

- (id)initWithRom:(iNDSROM *)rom {
    if (self = [super init]) {
        self.settings = [iNDSSettings sharedInstance];
        _emulatorView = [[iNDSEmulationView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
        
        [self initWithLanguage:-1];
        [self loadRom:rom.path];
        
        self.coreLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(executeFrame)];
        self.coreLink.paused = NO;
        self.coreLink.preferredFramesPerSecond = 60;
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            [NSThread sleepForTimeInterval:1];
//            [self.coreLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
//            [[NSRunLoop currentRunLoop] run];
            for (int i = 0; i < 6000; i++) {
                [self executeFrame];
            }
        });
        self.speed = 1;
    }
    return self;
}

- (void)initWithLanguage:(int)lang
{
    NSLog(@"Initiating DeSmuME");
    //oglrender_init = android_opengl_init;
    
    path.ReadPathSettings();
    //    if (video.layout > 2)
    //    {
    //        video.layout = video.layout_old = 0;
    //    }
    
    EMU_loadDefaultSettings();
    
    Desmume_InitOnce();
    //gpu_SetRotateScreen(video.rotation);
    NDS_FillDefaultFirmwareConfigData(&fw_config);
    //Hud.reset();
    
    INFO("Init NDS");
    
    NDS_Init();
    
    //osd->singleScreen = true;
    cur3DCore = 1;
    NDS_3D_ChangeCore(cur3DCore); //OpenGL
    
    //    LOG("Init sound core\n");
    //    SPU_ChangeSoundCore(SNDCORE_COREAUDIO, DESMUME_SAMPLE_RATE*8/60);
    //
    static const char* nickname = "iNDS";
    fw_config.nickname_len = strlen(nickname);
    for(int i = 0 ; i < fw_config.nickname_len ; ++i) {
        fw_config.nickname[i] = nickname[i];
    }
    
    static const char* message = "iNDS is the best!";
    fw_config.message_len = strlen(message);
    for(int i = 0 ; i < fw_config.message_len ; ++i) {
        fw_config.message[i] = message[i];
    }
    
    fw_config.language = lang < 0 ? NDS_FW_LANG_ENG : lang;
    fw_config.fav_colour = 15;
    fw_config.birth_month = 2;
    fw_config.birth_day = 17;
    fw_config.ds_type = NDS_CONSOLE_TYPE_LITE;
    fw_config.language = 1;
    
    //video.setfilter(video.NONE);
    NDS_CreateDummyFirmware(&fw_config);
}

- (void)skipFrame
{
    NDS_SkipNextFrame();
    [self executeFrame];
}

CFTimeInterval lastExecute = 0;
int xx = 0;
- (void)executeFrame
{
    CFTimeInterval duration = CACurrentMediaTime() - lastExecute;
    lastExecute = CACurrentMediaTime();
    _fps = _fps * 0.99 + (1.0 / duration) * 0.01;
    
    NDS_beginProcessingInput();
    NDS_endProcessingInput();
//    timeOverlap += self.coreLink.duration;
//    timeOverlap = MIN(timeOverlap, FRAME_TIME * 4);
//
//    NSInteger framesRendered = 0;
//    for (;timeOverlap >= FRAME_TIME; timeOverlap -= FRAME_TIME) {
//        NDS_exec<false>();
//        framesRendered++;
//    }
    for (int i = 0; i < self.speed; i++) {
        NDS_exec<false>();
    }
    
    [_emulatorView updateDisplay];
    
    if (xx++ % 60 == 0) {
        NSLog(@"FPS: %f", _fps);
    }
    //SPU_Emulate_user();
}

- (void)resumeEmulator {
    self.coreLink.paused = NO;
}

- (void)pauseEmulator {
    self.coreLink.paused = YES;
}

- (iNDSEmulationView *)emulatorView {
    return _emulatorView;
}

- (CGFloat)fps {
    return _fps;
}

- (BOOL)loadRom:(NSString*)path
{
    NSLog(@"Loading Rom: %@", path);
    return NDS_LoadROM([path UTF8String]) > 0;
}



@end

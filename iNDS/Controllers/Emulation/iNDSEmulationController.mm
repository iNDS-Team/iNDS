//
//  iNDSEmulationController.m
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationController.h"
#import "iNDSEmulationView.h"
#import "iNDSROM.h"
#import "iNDSSettings.h"

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

@interface iNDSEmulationController () {
    struct NDS_fw_config_data fw_config;
    
    iNDSEmulationView   *_emulatorView;
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
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            [self.coreLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
            [[NSRunLoop currentRunLoop] run];
        });
        
    }
    return self;
}

- (void)loadSettings {
    CommonSettings.num_cores = (int)sysconf( _SC_NPROCESSORS_ONLN );
    NSLog(@"%i cores detected", CommonSettings.num_cores);
    CommonSettings.advanced_timing = false;
    CommonSettings.cheatsDisable = false;
    CommonSettings.autodetectBackupMethod = 0;
    CommonSettings.use_jit = false;

    CommonSettings.hud.FpsDisplay = true;
    CommonSettings.hud.FrameCounterDisplay = false;
    CommonSettings.hud.ShowInputDisplay = false;
    CommonSettings.hud.ShowGraphicalInputDisplay = false;
    CommonSettings.hud.ShowLagFrameCounter = false;
    CommonSettings.hud.ShowMicrophone = false;
    CommonSettings.hud.ShowRTC = false;
    
    CommonSettings.micMode = TCommonSettings::Physical;
    CommonSettings.showGpu.main = 1;
    CommonSettings.showGpu.sub = 1;
    
    // Wifif
    CommonSettings.wifi.mode = 0;
    CommonSettings.wifi.infraBridgeAdapter = 0;
    
    // Graphics
    CommonSettings.GFX3D_HighResolutionInterpolateColor = 1;
    CommonSettings.GFX3D_EdgeMark = 0;
    CommonSettings.GFX3D_Fog = 1;
    CommonSettings.GFX3D_Texture = 1;
    CommonSettings.GFX3D_LineHack = 0;
    CommonSettings.GFX3D_Zelda_Shadow_Depth_Hack = 1;
    
    // Sound
    CommonSettings.spuInterpolationMode = SPUInterpolation_Cosine;
    CommonSettings.spu_advanced = false;
    //CommonSettings.SPU_sync_mode = SPU_SYNC_MODE_SYNCHRONOUS;
    //CommonSettings.SPU_sync_method = SPU_SYNC_METHOD_N;
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
    
    [self loadSettings];
    
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

- (void)executeFrame
{
   // [cdsController flush];
    NSLog(@"Executing frame");
    NDS_beginProcessingInput();
    NDS_endProcessingInput();
    NDS_exec<false>();
    [_emulatorView updateDisplay];
    
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

- (BOOL)loadRom:(NSString*)path
{
    NSLog(@"Loading Rom: %@", path);
    return NDS_LoadROM([path UTF8String]) > 0;
}



@end

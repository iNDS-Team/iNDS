//
//  emu.cpp
//  iNDS
//
//  Created by rock88 on 18/12/2012.
//  Copyright (c) 2012 Homebrew. All rights reserved.
//

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
#include "video.h"
#include "throttle.h"
#include "sndcoreaudio.h"
#include "cheatSystem.h"
#include "slot1.h"
#include "version.h"
#include "metaspu.h"
#include "GPU.h"

#define LOGI(...) printf(__VA_ARGS__);printf("\n")

CACHE_ALIGN u8 __GPU_screen[2][4*256*192];

extern VideoInfo video;

bool NDS_Pause(bool showMsg = true);
void NDS_UnPause(bool showMsg = true);

struct NDS_fw_config_data fw_config;

GPU3DInterface *core3DList[] = {
	&gpu3DNull,
	//&gpu3Dgl,
	&gpu3DRasterize,
	NULL
};

SoundInterface_struct *SNDCoreList[] = {
	&SNDDummy,
	&SNDCoreAudio,
	NULL
};

struct MainLoopData
{
	u64 freq;
	int framestoskip;
	int framesskipped;
	int skipnextframe;
	u64 lastticks;
	u64 curticks;
	u64 diffticks;
	u64 fpsticks;
	int fps;
	int fps3d;
	int fpsframecount;
	int toolframecount;
}  mainLoopData = {0};

VideoInfo video;


bool useMmapForRomLoading = false;
volatile bool execute = true;
bool autoframeskipenab=1;
int frameskiprate=3;
int lastskiprate=0;
bool continuousframeAdvancing = false;
//int FastForward=5;
bool frameAdvance = false;
bool FrameLimit = true;
int emu_paused = 0;
volatile bool paused = false;
bool enableMicrophone = true;
volatile bool pausedByMinimize = false;
volatile bool soundEnabled = true;

void EMU_init(int lang)
{
	//oglrender_init = android_opengl_init;
	
	path.ReadPathSettings();
	if (video.layout > 2)
	{
		video.layout = video.layout_old = 0;
	}
	
	EMU_loadSettings();
    
	Desmume_InitOnce();
	//gpu_SetRotateScreen(video.rotation);
	NDS_FillDefaultFirmwareConfigData(&fw_config);
	//Hud.reset();
	
	INFO("Init NDS");
	/*
	switch (slot1_device_type)
	{
		case NDS_SLOT1_NONE:
		case NDS_SLOT1_RETAIL:
		case NDS_SLOT1_R4:
		case NDS_SLOT1_RETAIL_NAND:
			break;
		default:
			slot1_device_type = NDS_SLOT1_RETAIL;
			break;
	}
	
	switch (addon_type)
	{
        case NDS_ADDON_NONE:
            break;
        case NDS_ADDON_CFLASH:
            break;
        case NDS_ADDON_RUMBLEPAK:
            break;
        case NDS_ADDON_GBAGAME:
            if (!strlen(GBAgameName))
            {
                addon_type = NDS_ADDON_NONE;
                break;
            }
            // TODO: check for file exist
            break;
        case NDS_ADDON_GUITARGRIP:
            break;
        case NDS_ADDON_EXPMEMORY:
            break;
        case NDS_ADDON_PIANO:
            break;
        case NDS_ADDON_PADDLE:
            break;
        default:
            addon_type = NDS_ADDON_NONE;
            break;
	}
    
	//!slot1Change((NDS_SLOT1_TYPE)slot1_device_type);
	addonsChangePak(addon_type);
    */
	NDS_Init();
	
	//osd->singleScreen = true;
	cur3DCore = 1;
	NDS_3D_ChangeCore(cur3DCore); //OpenGL
	
	LOG("Init sound core\n");
	SPU_ChangeSoundCore(SNDCORE_COREAUDIO, DESMUME_SAMPLE_RATE*8/60);
	
	static const char* nickname = "iNDS";
	fw_config.nickname_len = strlen(nickname);
	for(int i = 0 ; i < fw_config.nickname_len ; ++i)
		fw_config.nickname[i] = nickname[i];
    
	static const char* message = "iNDS is the best!";
	fw_config.message_len = strlen(message);
	for(int i = 0 ; i < fw_config.message_len ; ++i)
		fw_config.message[i] = message[i];
	
	fw_config.language = lang < 0 ? NDS_FW_LANG_ENG : lang;
	fw_config.fav_colour = 15;
	fw_config.birth_month = 2;
	fw_config.birth_day = 17;
	fw_config.ds_type = NDS_CONSOLE_TYPE_LITE;
    
	video.setfilter(video.NONE);
	
	NDS_CreateDummyFirmware(&fw_config);
	
	InitSpeedThrottle();
	
	mainLoopData.freq = 1000;
	mainLoopData.lastticks = GetTickCount();
}

void EMU_loadSettings()
{
    CommonSettings.num_cores = (int) sysconf( _SC_NPROCESSORS_ONLN );
	LOGI("%i cores detected", CommonSettings.num_cores);
	CommonSettings.cheatsDisable = false;
	CommonSettings.autodetectBackupMethod = 0;
	video.rotation =  0;
	video.rotation_userset = video.rotation;
	video.layout_old = video.layout = 0;
	video.swap = 0;
	CommonSettings.hud.FpsDisplay = true;
	CommonSettings.hud.FrameCounterDisplay = false;
	CommonSettings.hud.ShowInputDisplay = false;
	CommonSettings.hud.ShowGraphicalInputDisplay = false;
	CommonSettings.hud.ShowLagFrameCounter = false;
	CommonSettings.hud.ShowMicrophone = false;
	CommonSettings.hud.ShowRTC = false;
    CommonSettings.micMode = TCommonSettings::Physical;
	video.screengap = 0;
	CommonSettings.showGpu.main = 1;
	CommonSettings.showGpu.sub = 1;
	CommonSettings.spu_advanced = false;
	CommonSettings.advanced_timing = true;
	CommonSettings.GFX3D_Zelda_Shadow_Depth_Hack = 1;
	CommonSettings.wifi.mode = 0;
	CommonSettings.wifi.infraBridgeAdapter = 0;
    autoframeskipenab = true;
	frameskiprate = 2;
//    CommonSettings.CpuMode = 1;
	CommonSettings.spuInterpolationMode = SPUInterpolation_Cosine; //Will
	CommonSettings.GFX3D_HighResolutionInterpolateColor = 1;
	CommonSettings.GFX3D_EdgeMark = 0;
	CommonSettings.GFX3D_Fog = 1;
	CommonSettings.GFX3D_Texture = 1;
	CommonSettings.GFX3D_LineHack = 0;
	useMmapForRomLoading = true;
	fw_config.language = 1;
	enableMicrophone = true;
}

void iNDS_unpause()
{
	if(!execute) NDS_Pause(false);
	if (emu_paused && autoframeskipenab && frameskiprate) AutoFrameSkip_IgnorePreviousDelay();
	NDS_UnPause();
}

bool doRomLoad(const char* path, const char* logical)
{
	if(NDS_LoadROM(path, logical) >= 0)
	{
		INFO("Loading %s was successful\n",path);
		iNDS_unpause();
		if (autoframeskipenab && frameskiprate) AutoFrameSkip_IgnorePreviousDelay();
		return true;
	}
	return false;
}

bool EMU_loadRom(const char* path)
{
    paused = 0;
	return doRomLoad(path, path);
}

bool NDS_Pause(bool showMsg)
{
	if(paused == 1) return false;
	emu_halt();
	paused = TRUE;
	SPU_Pause(1);
	while (!paused) {}
	if (showMsg) INFO("Emulation paused\n");
    
	return true;
}

void NDS_UnPause(bool showMsg)
{
	if (/*romloaded &&*/ paused)
	{
		paused = FALSE;
		pausedByMinimize = FALSE;
		execute = TRUE;
		SPU_Pause(0);
		if (showMsg) INFO("Emulation unpaused\n");
        
	}
}

void EMU_pause(bool pause)
{
    if (pause) NDS_Pause();
    else NDS_UnPause();
}

void EMU_change3D(int type)
{
	NDS_3D_ChangeCore(cur3DCore = type);
}

void EMU_changeSound(int type)
{
	SPU_ChangeSoundCore(type, DESMUME_SAMPLE_RATE*8/60);
}

void EMU_setSynchMode(bool enabled)
{
    if (enabled == true) {
        SPU_SetSynchMode(ESynchMode_Synchronous,ESynchMethod_N);
    } else {
        SPU_SetSynchMode(ESynchMode_DualSynchAsynch,ESynchMethod_N);
    }
}

void EMU_enableSound(bool enabled)
{
/*
 SPU seems to need to be kickstarted. If left disabled when initializing the ROM,
 then we can't expect to magically enable it partway through. To solve this, we
 just let it run for a cycle if we disable sound.
 
 More information see iNDS-Team/iNDS#35
 */
    if (!enabled) SPU_Emulate_user(true);
    soundEnabled = enabled;
}

void EMU_setFrameSkip(int skip)
{
    if (skip == -1) {
        // auto
        autoframeskipenab = true;
        frameskiprate = 1;
    } else {
        autoframeskipenab = false;
        frameskiprate = skip;
    }
}

void EMU_setCPUMode(int cpuMode)
{
//    CommonSettings.CpuMode = cpuMode;
}

void EMU_setAdvancedBusTiming(bool mode) {
    CommonSettings.advanced_timing = mode;
}

void EMU_setDepthComparisonThreshold(int depth) {
    CommonSettings.GFX3D_Zelda_Shadow_Depth_Hack = depth;
}

void EMU_runCore()
{
	NDS_beginProcessingInput();
	NDS_endProcessingInput();
	NDS_exec<false>();
    if (soundEnabled) SPU_Emulate_user(true);
}

void iNDS_user()
{
	//Hud.fps = mainLoopData.fps;
	//Hud.fps3d = mainLoopData.fps3d;
    
	gfx3d.frameCtrRaw++;
	if(gfx3d.frameCtrRaw == 60) {
		mainLoopData.fps3d = gfx3d.frameCtr;
		gfx3d.frameCtrRaw = 0;
		gfx3d.frameCtr = 0;
	}
    
	mainLoopData.toolframecount++;
    
	//Update_RAM_Search(); // Update_RAM_Watch() is also called.
    
	mainLoopData.fpsframecount++;
	mainLoopData.curticks = GetTickCount();
	bool oneSecond = mainLoopData.curticks >= mainLoopData.fpsticks + mainLoopData.freq;
	if(oneSecond) // TODO: print fps on screen in DDraw
	{
		mainLoopData.fps = mainLoopData.fpsframecount;
		mainLoopData.fpsframecount = 0;
		mainLoopData.fpsticks = GetTickCount();
	}
}

bool EMU_frameSkip(bool force)
{
    if (force) {
        NDS_SkipNextFrame();
        return true;
    }
    bool skipped;
    //Change in skip rate
	if(lastskiprate != frameskiprate)
	{
		lastskiprate = frameskiprate;
		mainLoopData.framestoskip = 0; // otherwise switches to lower frameskip rates will lag behind
	}
    
    
    //Load a frame
	if(!mainLoopData.skipnextframe)
	{
		mainLoopData.framesskipped = 0;
        
		if (mainLoopData.framestoskip > 0)
			mainLoopData.skipnextframe = 1;
        skipped = false;
	}
	else //Skip
	{
		mainLoopData.framestoskip--;
        
		if (mainLoopData.framestoskip < 1)
			mainLoopData.skipnextframe = 0;
		else
			mainLoopData.skipnextframe = 1;
        
		mainLoopData.framesskipped++;
        
		NDS_SkipNextFrame();
        skipped = true;
	}
    
    if (mainLoopData.framestoskip < 1) {
        mainLoopData.framestoskip += frameskiprate;
	}
    
	if(execute && emu_paused && !frameAdvance)
	{
		// safety net against running out of control in case this ever happens.
		NDS_UnPause(); NDS_Pause();
	}
    
	//ServiceDisplayThreadInvocations();
    return skipped;
}

int EMU_runOther()
{
	if(execute)
	{
		iNDS_user();
		EMU_frameSkip(false);
		return mainLoopData.fps > 0 ? mainLoopData.fps : 1;
	}
	return 1;
}

bool EMU_loadState(const char *filename)
{
    return savestate_load(filename);
}

bool EMU_saveState(const char *filename)
{
    return savestate_save(filename);
}

void* EMU_getVideoBuffer(size_t *outSize)
{
    if (outSize) *outSize = video.size();
    //return video.buffer;
    // Will
    video.filter();
    return video.finalBuffer();
}

CACHE_ALIGN  u32 RGbA8_Buffer[256*192*2];

u32 *EMU_RBGA8Buffer()
{
    const int size = 192 * 256 * 2;
    
    u16* src = (u16*)GPU_screen;
    for(int i=0;i<size;++i)
        RGbA8_Buffer[i] = 0xFF000000 | RGB15TO32_NOALPHA(src[i]);
    return RGbA8_Buffer;
}

void EMU_copyMasterBuffer()
{
    video.copyBuffer(GPU_screen);
}

void EMU_touchScreenTouch(int x, int y)
{
	if(x<0) x = 0; else if(x>255) x = 255;
	if(y<0) y = 0; else if(y>192) y = 192;
	NDS_setTouchPos(x,y);
}

void EMU_touchScreenRelease()
{
	NDS_releaseTouch();
}

void EMU_setWorkingDir(const char* path)
{
    strncpy(PathInfo::pathToModule, path, MAX_PATH);
}

void EMU_closeRom()
{
	NDS_FreeROM();
	execute = false;
	//Hud.resetTransient();
	NDS_Reset();
}

static BOOL _b[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define all_button _b[0], _b[1], _b[2], _b[3], _b[4], _b[5], _b[6], _b[7], _b[8], _b[9], _b[10], _b[11], _b[12], _b[13]

void EMU_setButtons(int l, int r, int up, int down, int left, int right, int a, int b, int x, int y, int start, int select)
{
	NDS_setPad(right, left, down, up, select, start, b, a, y, x, l, r, _b[12], _b[13]);
}

void EMU_buttonDown(BUTTON_ID button)
{
    _b[button] = true;
    NDS_setPad(all_button);
}

void EMU_buttonUp(BUTTON_ID button)
{
    _b[button] = false;
    NDS_setPad(all_button);
}

void EMU_setDPad(bool up, bool down, bool left, bool right)
{
    _b[BUTTON_UP] = !!up;
    _b[BUTTON_DOWN] = !!down;
    _b[BUTTON_LEFT] = !!left;
    _b[BUTTON_RIGHT] = !!right;
    NDS_setPad(all_button);
}

void EMU_setABXY(bool a, bool b, bool x, bool y)
{
    _b[BUTTON_A] = !!a;
    _b[BUTTON_B] = !!b;
    _b[BUTTON_X] = !!x;
    _b[BUTTON_Y] = !!y;
    NDS_setPad(all_button);
}

void EMU_setFilter(int filter)
{
    video.setfilter(filter);
}

const char* EMU_version()
{
    return EMU_DESMUME_VERSION_STRING();
}


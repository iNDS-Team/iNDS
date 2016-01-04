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
#include "addons.h"
#include "slot1.h"
#include "saves.h"
#include "video.h"
#include "throttle.h"
#include "sndcoreaudio.h"
#include "cheatSystem.h"
#include "slot1.h"
#include "version.h"
#include "metaspu.h"


#define LOGI(...) printf(__VA_ARGS__);printf("\n")

CACHE_ALIGN u8 __GPU_screen[4*256*192];

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


void Mic_DeInit(){}
BOOL Mic_Init(){return true;}
void Mic_Reset(){}
u8 Mic_ReadSample(){return 0x99;}
void mic_savestate(EMUFILE* os){}
bool mic_loadstate(EMUFILE* is, int size){ return true;}

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
	*/
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
    
	NDS_Init();
	
	//osd->singleScreen = true;
	cur3DCore = 1;
	NDS_3D_ChangeCore(cur3DCore); //OpenGL
	
	LOG("Init sound core\n");
	SPU_ChangeSoundCore(SNDCORE_COREAUDIO, DESMUME_SAMPLE_RATE*8/60);
	
	static const char* nickname = "iNDS"; //TODO: Add firmware cfg in settings
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
    
	video.setfilter(video.NONE); //figure out why this doesn't seem to work (also add to cfg)
	
	NDS_CreateDummyFirmware(&fw_config);
	
	InitSpeedThrottle();
	
	mainLoopData.freq = 1000;
	mainLoopData.lastticks = GetTickCount();
}

void EMU_loadSettings()
{
	CommonSettings.num_cores = sysconf( _SC_NPROCESSORS_ONLN );
	LOGI("%i cores detected", CommonSettings.num_cores);
	CommonSettings.advanced_timing = false;
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
	CommonSettings.advanced_timing = false;
	CommonSettings.GFX3D_Zelda_Shadow_Depth_Hack = 0;
	CommonSettings.wifi.mode = 0;
	CommonSettings.wifi.infraBridgeAdapter = 0;
    autoframeskipenab = true;
	frameskiprate = 2;
    CommonSettings.CpuMode = 1;
	CommonSettings.spuInterpolationMode = SPUInterpolation_Cosine; //Will
	CommonSettings.GFX3D_HighResolutionInterpolateColor = 1;
	CommonSettings.GFX3D_EdgeMark = 0;
	CommonSettings.GFX3D_Fog = 1;
	CommonSettings.GFX3D_Texture = 1;
	CommonSettings.GFX3D_LineHack = 0;
	useMmapForRomLoading = true;
	fw_config.language = 1;
	enableMicrophone = true; //doesn't do anything yet
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
	return doRomLoad(path, path);
}

bool NDS_Pause(bool showMsg)
{
	if(paused) return false;
    
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
    CommonSettings.CpuMode = cpuMode;
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
    
    //Don't think this does very much
	return;
    if(nds.idleFrameCounter==0 || oneSecond)
	{
		//calculate a 16 frame arm9 load average
		for(int cpu=0;cpu<2;cpu++)
		{
			int load = 0;
			//printf("%d: ",cpu);
			for(int i=0;i<16;i++)
			{
				//blend together a few frames to keep low-framerate games from having a jittering load average
				//(they will tend to work 100% for a frame and then sleep for a while)
				//4 frames should handle even the slowest of games
				s32 sample =
                nds.runCycleCollector[cpu][(i+0+nds.idleFrameCounter)&15]
				+	nds.runCycleCollector[cpu][(i+1+nds.idleFrameCounter)&15]
				+	nds.runCycleCollector[cpu][(i+2+nds.idleFrameCounter)&15]
				+	nds.runCycleCollector[cpu][(i+3+nds.idleFrameCounter)&15];
				sample /= 4;
				load = load/8 + sample*7/8;
			}
			//printf("\n");
			load = std::min(100,std::max(0,(int)(load*100/1120380)));
			//Hud.cpuload[cpu] = load;
		}
	}
    
	//Hud.cpuloopIterationCount = nds.cpuloopIterationCount;
}

static void iNDS_throttle(bool allowSleep = true, int forceFrameSkip = -1)
{
	int skipRate = (forceFrameSkip < 0) ? frameskiprate : forceFrameSkip;
	int ffSkipRate = (forceFrameSkip < 0) ? 9 : forceFrameSkip;
    
    //Change in skip rate
	if(lastskiprate != skipRate)
	{
		lastskiprate = skipRate;
		mainLoopData.framestoskip = 0; // otherwise switches to lower frameskip rates will lag behind
	}
    
    
    //Load a frame
	if(!mainLoopData.skipnextframe || forceFrameSkip == 0 || frameAdvance || (continuousframeAdvancing && !FastForward))
	{
		mainLoopData.framesskipped = 0;
        
		if (mainLoopData.framestoskip > 0)
			mainLoopData.skipnextframe = 1;
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
	}
    
    if ((/*autoframeskipenab && frameskiprate ||*/ FrameLimit) && allowSleep)
	{
		SpeedThrottle();
	}
    
	if (autoframeskipenab && frameskiprate)
	{
		if(!frameAdvance && !continuousframeAdvancing)
		{
			AutoFrameSkip_NextFrame();
			if (mainLoopData.framestoskip < 1)
				mainLoopData.framestoskip += AutoFrameSkip_GetSkipAmount(0,skipRate);
		}
	}
	else
	{
		if (mainLoopData.framestoskip < 1)
			mainLoopData.framestoskip += skipRate;
	}
    
	if (frameAdvance && allowSleep)
	{
		frameAdvance = false;
		emu_halt();
		SPU_Pause(1);
	}
	if(execute && emu_paused && !frameAdvance)
	{
		// safety net against running out of control in case this ever happens.
		NDS_UnPause(); NDS_Pause();
	}
    
	//ServiceDisplayThreadInvocations();
}

int EMU_runOther()
{
	if(execute)
	{
		iNDS_user();
		iNDS_throttle();
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
    return video.buffer;
}

void EMU_copyMasterBuffer()
{
	video.srcBuffer = (u8*)GPU_screen;
	
	//convert pixel format to 32bpp for compositing
	//why do we do this over and over? well, we are compositing to
	//filteredbuffer32bpp, and it needs to get refreshed each frame..
	const int size = video.size();
	u16* src = (u16*)video.srcBuffer;
    u32* dest = video.buffer;
    for(int i=0;i<size;++i)
        *dest++ = 0xFF000000 | RGB15TO32_NOALPHA(src[i]);
	
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

static BOOL _b[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
#define all_button _b[0], _b[1], _b[2], _b[3], _b[4], _b[5], _b[6], _b[7], _b[8], _b[9], _b[10], _b[11]

void EMU_setButtons(int l, int r, int up, int down, int left, int right, int a, int b, int x, int y, int start, int select)
{
	NDS_setPad(right, left, down, up, select, start, b, a, y, x, l, r, false, false);
}

void EMU_buttonDown(BUTTON_ID button)
{
    _b[button] = true;
    NDS_setPad(all_button, false, false);
}

void EMU_buttonUp(BUTTON_ID button)
{
    _b[button] = false;
    NDS_setPad(all_button, false, false);
}

void EMU_setDPad(bool up, bool down, bool left, bool right)
{
    _b[BUTTON_UP] = !!up;
    _b[BUTTON_DOWN] = !!down;
    _b[BUTTON_LEFT] = !!left;
    _b[BUTTON_RIGHT] = !!right;
    NDS_setPad(all_button, false, false);
}

void EMU_setABXY(bool a, bool b, bool x, bool y)
{
    _b[BUTTON_A] = !!a;
    _b[BUTTON_B] = !!b;
    _b[BUTTON_X] = !!x;
    _b[BUTTON_Y] = !!y;
    NDS_setPad(all_button, false, false);
}


const char* EMU_version()
{
    return EMU_DESMUME_VERSION_STRING();
}


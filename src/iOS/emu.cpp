//
//  emu.cpp
//  iNDS
//
//  Created by Will Cobb on 1/10/2017.
//  Copyright (c) 2017 iNDS. All rights reserved.
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
#include "videofilter.h"
#include "cheatSystem.h"
#include "slot1.h"
#include "version.h"
#include "metaspu.h"
#include "GPU.h"

#define LOGI(...) printf(__VA_ARGS__);printf("\n")

volatile bool execute = true;

bool NDS_Pause(bool showMsg = true);
void NDS_UnPause(bool showMsg = true);

struct NDS_fw_config_data fw_config;
bool soundEnabled;

GPU3DInterface *core3DList[] = {
	&gpu3DNull,
	//&gpu3Dgl,
	&gpu3DRasterize,
	NULL
};

SoundInterface_struct *SNDCoreList[] = {
	&SNDDummy,
	//&SNDCoreAudio,
	NULL
};

void EMU_loadDefaultSettings() {
    CommonSettings.num_cores = (int)sysconf( _SC_NPROCESSORS_ONLN );
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
    CommonSettings.GFX3D_HighResolutionInterpolateColor = 0;
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

CACHE_ALIGN  u32 RGbA8_Buffer[256*192*2];

u32 *EMU_RBGA8Buffer()
{
    const int size = 192 * 256 * 2;
    
    u16* src = (u16*)GPU_screen;
    for(int i=0;i<size;++i)
        RGbA8_Buffer[i] = 0xFF000000 | RGB15TO32_NOALPHA(src[i]);
    return RGbA8_Buffer;
}

bool EMU_loadRom(const char *filepath) {
    return NDS_LoadROM(filepath) > 0;
}

void EMU_change3D(int type) {
	NDS_3D_ChangeCore(cur3DCore = type);
}

void EMU_changeSound(int type) {
	SPU_ChangeSoundCore(type, DESMUME_SAMPLE_RATE*8/60);
}

void EMU_setSynchMode(bool enabled) {
    if (enabled == true) {
        SPU_SetSynchMode(ESynchMode_Synchronous,ESynchMethod_N);
    } else {
        SPU_SetSynchMode(ESynchMode_DualSynchAsynch,ESynchMethod_N);
    }
}

void EMU_runCore() {
	NDS_beginProcessingInput();
	NDS_endProcessingInput();
	NDS_exec<false>();
    if (soundEnabled) SPU_Emulate_user(true);
}

bool EMU_loadState(const char *filename) {
    return savestate_load(filename);
}

bool EMU_saveState(const char *filename) {
    return savestate_save(filename);
}

void EMU_touchScreenTouch(int x, int y) {
	if(x<0) x = 0; else if(x>255) x = 255;
	if(y<0) y = 0; else if(y>192) y = 192;
	NDS_setTouchPos(x,y);
}

void EMU_touchScreenRelease() {
	NDS_releaseTouch();
}

void EMU_setWorkingDir(const char* path) {
    //strncpy(PathInfo::pathToModule, path, MAX_PATH);
}

void EMU_closeRom() {
	NDS_FreeROM();
	execute = false;
	NDS_Reset();
}

static BOOL _b[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define all_button _b[0], _b[1], _b[2], _b[3], _b[4], _b[5], _b[6], _b[7], _b[8], _b[9], _b[10], _b[11], _b[12], _b[13]

void EMU_setButtons(int l, int r, int up, int down, int left, int right, int a, int b, int x, int y, int start, int select) {
	NDS_setPad(right, left, down, up, select, start, b, a, y, x, l, r, _b[12], _b[13]);
}

void EMU_buttonDown(BUTTON_ID button) {
    _b[button] = true;
    NDS_setPad(all_button);
}

void EMU_buttonUp(BUTTON_ID button) {
    _b[button] = false;
    NDS_setPad(all_button);
}

void EMU_setLid(bool closed) {
    _b[BUTTON_LID] = closed;
    NDS_setPad(all_button);
}

void EMU_setDPad(bool up, bool down, bool left, bool right) {
    _b[BUTTON_UP] = !!up;
    _b[BUTTON_DOWN] = !!down;
    _b[BUTTON_LEFT] = !!left;
    _b[BUTTON_RIGHT] = !!right;
    NDS_setPad(all_button);
}

void EMU_setABXY(bool a, bool b, bool x, bool y) {
    _b[BUTTON_A] = !!a;
    _b[BUTTON_B] = !!b;
    _b[BUTTON_X] = !!x;
    _b[BUTTON_Y] = !!y;
    NDS_setPad(all_button);
}

void EMU_setFilter(int filter) {
    //video.setfilter(filter);
}

const char* EMU_version() {
    return EMU_DESMUME_VERSION_STRING();
}


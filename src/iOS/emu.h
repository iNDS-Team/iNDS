//
//  emu.h
//  iNDS
//
//  Created by Will Cobb on 1/10/2017.
//  Copyright (c) 2017 iNDS. All rights reserved.
//

#ifndef _EMU_H_
#define _EMU_H_

#include <iostream>

extern volatile bool execute;

typedef unsigned char u8;
typedef unsigned int u32;

typedef enum {
	BUTTON_RIGHT = 0,
    BUTTON_LEFT = 1,
	BUTTON_DOWN = 2,
	BUTTON_UP = 3,
    BUTTON_SELECT = 4,
    BUTTON_START = 5,
	BUTTON_B = 6,
	BUTTON_A = 7,
	BUTTON_Y = 8,
	BUTTON_X = 9,
	BUTTON_L = 10,
	BUTTON_R = 11,
    BUTTON_DEBUG = 12,
    BUTTON_LID = 13,
} BUTTON_ID;


void EMU_init(int lang = -1);
u32 *EMU_RBGA8Buffer();
void EMU_loadDefaultSettings();
bool EMU_loadRom(const char* path);
void EMU_change3D(int type);
void EMU_changeSound(int type);
void EMU_enableSound(bool enable);
void EMU_setSynchMode(bool enabled);
void EMU_skipFrame();
void EMU_runCore();
void EMU_pause(bool pause);
bool EMU_loadState(const char *filename);
bool EMU_saveState(const char *filename);

void EMU_touchScreenTouch(int x, int y);
void EMU_touchScreenRelease();

void EMU_setWorkingDir(const char* path);
void EMU_closeRom();

void EMU_buttonDown(BUTTON_ID button);
void EMU_buttonUp(BUTTON_ID button);
void EMU_setDPad(bool up, bool down, bool left, bool right);
void EMU_setABXY(bool a, bool b, bool x, bool y);
void EMU_setFilter(int filter);

const char *EMU_version();

#endif 


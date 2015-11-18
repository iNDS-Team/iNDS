/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2008-2013 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SEQUENCER_H
#define _SEQUENCER_H

#include "types.h"
#include "emufile.h"

struct TSequenceItem
{
	u64 timestamp;
	u32 param;
	bool enabled;

	virtual void save(EMUFILE* os);
	virtual bool load(EMUFILE* is);
	FORCEINLINE bool isTriggered();
	FORCEINLINE u64 next();
};

struct TSequenceItem_GXFIFO : public TSequenceItem
{
	FORCEINLINE bool isTriggered();
	FORCEINLINE void exec();
	FORCEINLINE u64 next();
};

extern TSequenceItem_GXFIFO& sequencer_gfxfifo;
extern bool& sequencer_reschedule;
extern u64 nds_timer;

FORCEINLINE void NDS_Reschedule() 
{
	sequencer_reschedule = true;
}

#define NDS_RescheduleGXFIFO(cost)			\
{											\
	if(!sequencer_gfxfifo.enabled) {		\
		MMU.gfx3dCycles = nds_timer;		\
		sequencer_gfxfifo.enabled = true;	\
	}										\
	MMU.gfx3dCycles += cost;				\
	NDS_Reschedule();						\
}

#endif
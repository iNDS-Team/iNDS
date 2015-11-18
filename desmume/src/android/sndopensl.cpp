/*	sndopensl.cpp
	Copyright (C) 2012 Jeffrey Quesnelle

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

#include "SPU.h"
#include "sndopensl.h"
#include "main.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

int SNDOpenSLInit(int buffersize);
void SNDOpenSLDeInit();
void SNDOpenSLUpdateAudio(s16 *buffer, u32 num_samples);
u32 SNDOpenSLGetAudioSpace();
void SNDOpenSLMuteAudio();
void SNDOpenSLUnMuteAudio();
void SNDOpenSLSetVolume(int volume);
void SNDOpenSLClearAudioBuffer();

SoundInterface_struct SNDOpenSL = {
	SNDCORE_OPENSL,
	"OpenSL ES Sound Interface",
	SNDOpenSLInit,
	SNDOpenSLDeInit,
	SNDOpenSLUpdateAudio,
	SNDOpenSLGetAudioSpace,
	SNDOpenSLMuteAudio,
	SNDOpenSLUnMuteAudio,
	SNDOpenSLSetVolume,
	SNDOpenSLClearAudioBuffer,
};

#define FAILED(X) (X) != SL_RESULT_SUCCESS

SLObjectItf engineObject = NULL;
SLEngineItf engineEngine;
static SLObjectItf outputMixObject = NULL;
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLVolumeItf bqPlayerVolume;
static bool everEnqueued = false;


class SoundBuffer
{
public:
	SoundBuffer() : data(NULL)
	{
		reset();
	}
	~SoundBuffer()
	{
		reset();
	}
	void reset()
	{
		if(data != NULL)
			delete [] data;
		data = NULL;
		avail = true;
		samples = 0;
	}
	s16* data;
	bool avail;
	int samples;
};

static const int NUM_BUFFERS = 2;
SoundBuffer buffers[NUM_BUFFERS];
SoundBuffer empty;

static bool muted = false;
static bool currentlyPlaying = false;
static int soundbufsize = 0;
static int nextSoundBuffer = -1;
static SLmillibel maxVol;

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	SLresult result;
	if(buffers[nextSoundBuffer].avail)
	{
		result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, empty.data, soundbufsize);
		//LOGI("Returned empty sound buffer");
		return;
	}
	//LOGI("Returned sound buffer %d", nextSoundBuffer);
	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffers[nextSoundBuffer].data, buffers[nextSoundBuffer].samples * sizeof(s16) * 2);
	buffers[nextSoundBuffer == 0 ? 1 : 0].avail = true;
}

int SNDOpenSLInit(int buffersize)
{
	
	SLresult result;
	
	if(engineObject == NULL)
	{
		if(FAILED(result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL)))
			return -1;

		if(FAILED(result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE)))
			return -1;

		if(FAILED(result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine)))
			return -1;
	}

    const SLInterfaceID mixids[1] = {SL_IID_VOLUME};
    const SLboolean mixreq[1] = {SL_BOOLEAN_FALSE};
    if(FAILED(result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mixids, mixreq)))
		return -1;

    if(FAILED(result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE)))
		return -1;

	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
				   
	SLDataSource audioSrc = {&loc_bufq, &format_pcm};
	SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
	
    const SLInterfaceID playerids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean playerreq[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    if(FAILED(result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, playerids, playerreq)))
		return -1;
		
    if(FAILED(result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE)))
		return -1;

    if(FAILED((*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay)))
		return -1;

    if(FAILED(result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue)))
		return -1;
		
    if(FAILED((*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL)))
		return -1;
		
    if(FAILED(result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume)))
		return -1;
		
	if(FAILED(result = (*bqPlayerVolume)->GetMaxVolumeLevel(bqPlayerVolume, &maxVol)))
		return -1;
		
    if(FAILED(result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING)))
		return -1;
		
	buffers[0].reset();
	buffers[1].reset();
	soundbufsize = buffersize;
	if ((buffers[0].data = new s16[soundbufsize / sizeof(s16)]) == NULL)
		return -1;
	if ((buffers[1].data = new s16[soundbufsize / sizeof(s16)]) == NULL)
		return -1;
	if ((empty.data = new s16[soundbufsize / sizeof(s16)]) == NULL)
		return -1;

	memset(buffers[0].data, 0, soundbufsize);
	memset(buffers[1].data, 0, soundbufsize);
	memset(empty.data, 0, soundbufsize);
	muted = false;
	currentlyPlaying = false;
	LOGI("OpenSL created (for audio output)");
	return 0;
}

void SNDOpenSLDeInit()
{
	if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
		bqPlayerObject = NULL;
	}
	
	if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
	}
	
	/*if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
		engineObject = NULL;
	}*/
}

void SNDOpenSLUpdateAudio(s16 *buffer, u32 num_samples)
{
	for(int i = 0 ; i < NUM_BUFFERS ; ++i)
	{
		if(buffers[i].avail)
		{
			memcpy(buffers[i].data, buffer, sizeof(s16) * 2 * num_samples);
			int currentSoundBuffer = nextSoundBuffer;
			buffers[i].samples = num_samples;
			buffers[i].avail = false;
			nextSoundBuffer = i;
			if(!currentlyPlaying)
			{
				(*bqPlayerBufferQueue)->Clear(bqPlayerBufferQueue);
				bqPlayerCallback(bqPlayerBufferQueue, NULL);
				currentlyPlaying = true;
			}
			//LOGI("Copied %d samples to buffer %d", num_samples, nextSoundBuffer);
			return;
		}
	}
	//should never get here...
}

u32 SNDOpenSLGetAudioSpace()
{
	for(int i = 0 ; i < NUM_BUFFERS ; ++i)
	{
		if(buffers[i].avail)
			return soundbufsize / (sizeof(s16) * 2);
	}
	return 0;
}

void SNDOpenSLMuteAudio()
{
	(*bqPlayerVolume)->SetMute(bqPlayerVolume, true);
}

void SNDOpenSLUnMuteAudio()
{
	(*bqPlayerVolume)->SetMute(bqPlayerVolume, false);
}

void SNDOpenSLSetVolume(int volume)
{
	SLmillibel level = 0;
	if(volume == 100)
		level = maxVol;
	else if(volume > 0)
		level = maxVol / (100 - volume - 1);
	(*bqPlayerVolume)->SetVolumeLevel(bqPlayerVolume, level);
}

void SNDOpenSLClearAudioBuffer()
{
	for(int i = 0 ; i < NUM_BUFFERS ; ++i)
		memset(buffers[i].data, 0, soundbufsize);
}

void SNDOpenSLPaused(bool paused)
{
	if(bqPlayerPlay == NULL)
		return;
	(*bqPlayerPlay)->SetPlayState(bqPlayerPlay, paused ? SL_PLAYSTATE_PAUSED : SL_PLAYSTATE_PLAYING);
}

/*
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

/*
	The NDS microphone produces 8-bit sound sampled at 16khz.
	The sound data must be read sample-by-sample through the 
	ARM7 SPI device (touchscreen controller, channel 6).

	Note : I added these notes because the microphone isn't 
	documented on GBATek.
*/

#include "../mic.h"
#include "readwrite.h"
#include "main.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern SLObjectItf engineObject;
extern SLEngineItf engineEngine;

static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord = NULL;
static SLAndroidSimpleBufferQueueItf bqRecordBufferQueue;
static bool samplesReady = false;

#define MAX_NUMBER_INTERFACES 5 
#define MAX_NUMBER_INPUT_DEVICES 3

#define FAILED(X) (X) != SL_RESULT_SUCCESS

#define MIC_BUFSIZE 2048

static BOOL Mic_Inited = FALSE;

static s16 Mic_Buffer[2][MIC_BUFSIZE];

static int fullBuffer = -1;
static int recordingBuffer = -1;
static int fullBufferPos = 0;

#define JNI(X,...) Java_com_opendoorstudios_ds4droid_DeSmuME_##X(JNIEnv* env, jclass* clazz, __VA_ARGS__)
#define JNI_NOARGS(X) Java_com_opendoorstudios_ds4droid_DeSmuME_##X(JNIEnv* env, jclass* clazz)

bool enableMicrophone = false;

int lastBufferTime;

void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	int nextBuffer = recordingBuffer == 1 ? 0 : 1;
	(*bqRecordBufferQueue)->Enqueue(bqRecordBufferQueue, Mic_Buffer[nextBuffer], MIC_BUFSIZE * sizeof(s16));
	if(recordingBuffer != -1)
	{
		fullBufferPos = 0;
		fullBuffer = recordingBuffer;
		/*float bufferTime = GetTickCount() - lastBufferTime;
		bufferTime = 1000.0 / bufferTime;
		bufferTime *= MIC_BUFSIZE;*/
		//LOGI("Approx mic sample rate is %d", (int)bufferTime);
	}
	recordingBuffer = nextBuffer;
	//lastBufferTime = GetTickCount();
}

extern "C"
{

void JNI(setMicPaused, int set)
{
	if(Mic_Inited == TRUE)
	{
		if(set == 1)
		{
			(*recorderRecord)->SetRecordState(recorderRecord,SL_RECORDSTATE_PAUSED);
		}
		else 
		{
			Mic_Reset();
			(*recorderRecord)->SetRecordState(recorderRecord,SL_RECORDSTATE_RECORDING);
			bqRecorderCallback(bqRecordBufferQueue, NULL);
		}
	}

}

}

void Mic_DeInit()
{
	if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
		recorderObject = NULL;
	}
	
	/*if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
		engineObject = NULL;
	}*/
	
	Mic_Inited = FALSE;
}

BOOL Mic_Init()
{
	if(!enableMicrophone)
		return FALSE;
	if(Mic_Inited == TRUE)
		return TRUE;
	SLresult result;
	SLuint32 InputDeviceIDs[MAX_NUMBER_INPUT_DEVICES]; 
	SLint32   numInputs = 0; 
	SLboolean mic_available = SL_BOOLEAN_FALSE; 
	SLuint32 mic_deviceID = 0; 
	SLAudioIODeviceCapabilitiesItf AudioIODeviceCapabilitiesItf; 
	SLAudioInputDescriptor        AudioInputDescriptor;
	
	Mic_Inited = FALSE;
	
	//Some devices silently (literally haha) fail if you create multiple OpenSL ES instances.
	//So now we share it with the regular audio output driver
	if(engineObject == NULL)
	{
		if(FAILED(result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL)))
			return FALSE;

		if(FAILED(result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE)))
			return FALSE;

		if(FAILED(result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine)))
			return FALSE;
	}

		
	SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE,
                      SL_IODEVICE_AUDIOINPUT,
                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
	SLDataSource audioSrc = {&loc_dev, NULL};

	SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	
	//it seems that at least on my phone (galaxy nexus) the mic samples are always 16 bits, regardless of what you ask for
	//the sampling rate does seem to be honored, though
	SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
			  SL_PCMSAMPLEFORMAT_FIXED_16 , SL_PCMSAMPLEFORMAT_FIXED_16 ,
			  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
			  
	SLDataSink audioSnk = {&loc_bq, &format_pcm};
	
	const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	const SLboolean req[1] = {SL_BOOLEAN_TRUE};
	
	if(FAILED(result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 1, id, req)))
		return FALSE;

	if(FAILED(result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE)))
		return FALSE;

	if(FAILED(result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord)))
		return FALSE;
		
	if(FAILED(result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqRecordBufferQueue)))
		return FALSE;
		
	if(FAILED(result = (*bqRecordBufferQueue)->RegisterCallback(bqRecordBufferQueue, bqRecorderCallback, NULL)))
		return FALSE;
		
	if(FAILED(result = (*recorderRecord)->SetRecordState(recorderRecord,SL_RECORDSTATE_RECORDING)))
		return FALSE;
		
	Mic_Reset();
	
	bqRecorderCallback(bqRecordBufferQueue, NULL);
	
	LOGI("OpenSL created (for audio input)");
	return Mic_Inited = TRUE;
}

void Mic_Reset()
{
	recordingBuffer = fullBuffer = -1;
	fullBufferPos = 0;
	
	if(!Mic_Inited)
		return;

	memset(Mic_Buffer[0], 0x80, MIC_BUFSIZE);
	memset(Mic_Buffer[1], 0x80, MIC_BUFSIZE);
}

u8 Mic_ReadSample()
{
	u8 ret = 0;
	//static u8 print = 0;
	if(Mic_Inited == TRUE && fullBuffer != -1)
	{
		const s16 original = Mic_Buffer[fullBuffer][fullBufferPos];
		s16 sixteen = original;
		sixteen /= 256; //16 bit -> 8 bit
		sixteen += 128; //pcm 8 bit encoding midpoint is 127, while it's signed 0 for 16-bit
		ret = (u8)sixteen;
		if(fullBufferPos != (MIC_BUFSIZE-1))
			++fullBufferPos;
		/*if(++print == 10)
		{
			LOGI("Sound: original = %i, sixteen = %i, ret = %x", (int)original, (int)sixteen, (int)ret);
			print = 0;
		}*/
		
	}
	return ret;

}

void mic_savestate(EMUFILE* os)
{
	write32le(-1,os);
}

bool mic_loadstate(EMUFILE* is, int size)
{
	is->fseek(size, SEEK_CUR);
	return TRUE;
}
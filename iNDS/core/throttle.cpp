//THIS SPEED THROTTLE WAS TAKEN FROM FCEUX.
//Copyright (C) 2002 Xodnizel
//(the code might look quite different by now, though...)

/*
	Many Modifications Copyright (C) 2009-2010 DeSmuME team

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

#include "../common.h"
#include "../types.h"
#include "../debug.h"
#ifndef ANDROID
#include "../console.h"
#endif
#include "throttle.h"
#include "GPU_osd.h"

int FastForward=0;
static u64 tmethod,tfreq,afsfreq;
static const u64 core_desiredfps = 3920763; //59.8261
static u64 desiredfps = core_desiredfps;
static float desiredspf = 65536.0f / core_desiredfps;
#ifdef ANDROID
static int desiredFpsScalerIndex = 5;
#else
static int desiredFpsScalerIndex = GetPrivateProfileInt("Video","FPS Scaler Index", 5, IniName);
#endif

static u64 desiredFpsScalers [] = {
	1024,
	512, // 200%
	448, // 175%
	384, // 150%
	320, // 125%
	256, // 100%
	192,
	128, // 50%
	96,
	64, // 25%
	42,
	32,
	16,
};

#ifdef ANDROID
#include <sys/time.h>
#include <time.h>
unsigned int GetTickCount()
{
	timeval timer;
	gettimeofday(&timer, NULL);
	return (timer.tv_sec * 1000) + (timer.tv_usec/1000);
}

#if 0
unsigned long long RawGetTickCount()
{
	return clock();
}

unsigned long long RawGetTickPerSecond()
{
	return (unsigned long long)CLOCKS_PER_SEC;
}
#else
unsigned long long RawGetTickCount()
{
#ifdef IOS
    static mach_timebase_info_data_t timebase;
    uint64_t now = mach_absolute_time ();
    if (timebase.denom == 0) {
        mach_timebase_info (&timebase);
        timebase.denom *= 100; /* we return 100ns ticks */
    }
    return now * timebase.numer / timebase.denom;
#else
	timespec timer;
	clock_gettime(CLOCK_MONOTONIC, &timer);
	return ((unsigned long long)timer.tv_sec * 1000000000ULL) + timer.tv_nsec;
#endif
}

unsigned long long RawGetTickPerSecond()
{
	return 1000000000ULL;
}
#endif

void Sleep(int ms)
{
	usleep(ms * 1000);
}

#endif

void IncreaseSpeed(void) {

	if(desiredFpsScalerIndex)
		desiredFpsScalerIndex--;
	u64 desiredFpsScaler = desiredFpsScalers[desiredFpsScalerIndex];
	desiredfps = core_desiredfps * desiredFpsScaler / 256;
	desiredspf = 65536.0f / desiredfps;
	printf("Throttle fps scaling increased to: %f\n",desiredFpsScaler/256.0);
	osd->addLine("Target FPS up to %2.04f",desiredFpsScaler/256.0);
	#ifndef ANDROID
	WritePrivateProfileInt("Video","FPS Scaler Index", desiredFpsScalerIndex, IniName);
	#endif
}

void DecreaseSpeed(void) {

	if(desiredFpsScalerIndex != sizeof(desiredFpsScalers)/sizeof(desiredFpsScalers[0]) - 1)
		desiredFpsScalerIndex++;
	u64 desiredFpsScaler = desiredFpsScalers[desiredFpsScalerIndex];
	desiredfps = core_desiredfps * desiredFpsScaler / 256;
	desiredspf = 65536.0f / desiredfps;
	printf("Throttle fps scaling decreased to: %f\n",desiredFpsScaler/256.0);
	osd->addLine("Target FPS down to %2.04f",desiredFpsScaler/256.0);
#ifndef ANDROID
	WritePrivateProfileInt("Video","FPS Scaler Index", desiredFpsScalerIndex, IniName);
#endif
}

static u64 GetCurTime(void)
{
#ifndef ANDROID
	if(tmethod)
	{
		u64 tmp;
		QueryPerformanceCounter((LARGE_INTEGER*)&tmp);
		return tmp;
	}
	else
#endif
	{
		return (u64)GetTickCount();
	}
}

void InitSpeedThrottle(void)
{
	tmethod=0;
#ifndef ANDROID
	if(QueryPerformanceFrequency((LARGE_INTEGER*)&afsfreq))
		tmethod=1;
	else
#endif
		afsfreq=1000;
	tfreq = afsfreq << 16;

	AutoFrameSkip_IgnorePreviousDelay();
}

static void AutoFrameSkip_BeforeThrottle();

static u64 ltime;

void SpeedThrottle()
{
	AutoFrameSkip_BeforeThrottle();

waiter:
	if(FastForward)
		return;

	u64 ttime = GetCurTime();

	if((ttime - ltime) < (tfreq / desiredfps))
	{
		u64 sleepy;
		sleepy = (tfreq / desiredfps) - (ttime - ltime);  
		sleepy *= 1000;
		if(tfreq >= 65536)
			sleepy /= afsfreq;
		else
			sleepy = 0;
		if(sleepy >= 10)
			Sleep((sleepy / 2)); // reduce it further beacuse Sleep usually sleeps for more than the amount we tell it to
#ifndef ANDROID
		else if(sleepy > 0) // spin for <1 millisecond waits
			SwitchToThread(); // limit to other threads on the same CPU core for other short waits
#endif
		goto waiter;
	}
	if( (ttime-ltime) >= (tfreq*4/desiredfps))
		ltime=ttime;
	else
		ltime+=tfreq/desiredfps;
}


// auto frameskip

static u64 beginticks=0, endticks=0, preThrottleEndticks=0;
static float fSkipFrames = 0;
static float fSkipFramesError = 0;
static int lastSkip = 0;
static float lastError = 0;
static float integral = 0;

void AutoFrameSkip_IgnorePreviousDelay()
{
	beginticks = GetCurTime();

	// this seems to be a stable way of allowing the skip frames to
	// quickly adjust to a faster environment (e.g. after a loadstate)
	// without causing oscillation or a sudden change in skip rate
	fSkipFrames *= 0.5f;
}

static void AutoFrameSkip_BeforeThrottle()
{
	preThrottleEndticks = GetCurTime();
}

void AutoFrameSkip_NextFrame()
{
	endticks = GetCurTime();

	// calculate time since last frame
	u64 diffticks = endticks - beginticks;
	float diff = (float)diffticks / afsfreq;

	// calculate time since last frame not including throttle sleep time
	if(!preThrottleEndticks) // if we didn't throttle, use the non-throttle time
		preThrottleEndticks = endticks;
	u64 diffticksUnthrottled = preThrottleEndticks - beginticks;
	float diffUnthrottled = (float)diffticksUnthrottled / afsfreq;


	float error = diffUnthrottled - desiredspf;


	// reset way-out-of-range values
	if(diff > 1)
		diff = 1;
	if(error > 1 || error < -1)
		error = 0;
	if(diffUnthrottled > 1)
		diffUnthrottled = desiredspf;

	float derivative = (error - lastError) / diff;
	lastError = error;

	integral = integral + (error * diff);
	integral *= 0.99f; // since our integral isn't reliable, reduce it to 0 over time.

	// "PID controller" constants
	// this stuff is probably being done all wrong, but these seem to work ok
	static const float Kp = 40.0f;
	static const float Ki = 0.55f;
	static const float Kd = 0.04f;

	float errorTerm = error * Kp;
	float derivativeTerm = derivative * Kd;
	float integralTerm = integral * Ki;
	float adjustment = errorTerm + derivativeTerm + integralTerm;

	// apply the output adjustment
	fSkipFrames += adjustment;

	// if we're running too slowly, prevent the throttle from kicking in
	if(adjustment > 0 && fSkipFrames > 0)
		ltime-=tfreq/desiredfps;

	preThrottleEndticks = 0;
	beginticks = GetCurTime();
}

int AutoFrameSkip_GetSkipAmount(int min, int max)
{
	int rv = (int)fSkipFrames;
	fSkipFramesError += fSkipFrames - rv;

	// resolve accumulated fractional error
	// where doing so doesn't push us out of range
	while(fSkipFramesError >= 1.0f && rv <= lastSkip && rv < max)
	{
		fSkipFramesError -= 1.0f;
		rv++;
	}
	while(fSkipFramesError <= -1.0f && rv >= lastSkip && rv > min)
	{
		fSkipFramesError += 1.0f;
		rv--;
	}

	// restrict skip amount to requested range
	if(rv < min)
		rv = min;
	if(rv > max)
		rv = max;

	// limit maximum error accumulation (it's mainly only for fractional components)
	if(fSkipFramesError >= 4.0f)
		fSkipFramesError = 4.0f;
	if(fSkipFramesError <= -4.0f)
		fSkipFramesError = -4.0f;

	// limit ongoing skipframes to requested range + 1 on each side
	if(fSkipFrames < min-1)
		fSkipFrames = (float)min-1;
	if(fSkipFrames > max+1)
		fSkipFrames = (float)max+1;

//	printf("%d", rv);

	lastSkip = rv;
	return rv;
}



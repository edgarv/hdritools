/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#include "Timer.h"

#include <cassert>

#if defined(_WIN32)
// Exclude the MFC stuff and min/max macros
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
# include <windows.h>
#elif defined(__APPLE__)
// Info from the Mac OS X Reference Library (accessed February 2011)
// http://www.wand.net.nz/~smr26/wordpress/2009/01/19/monotonic-time-in-mac-os-x/
// http://developer.apple.com/library/mac/#qa/qa2004/qa1398.html
# include <CoreServices/CoreServices.h>
# include <mach/mach.h>
# include <mach/mach_time.h>
# include <unistd.h>
#else
// Assume POSIX. Check for good clocks support.
# include <ctime>
# if defined(CLOCK_MONOTONIC)
# define TIMER_CLOCK CLOCK_MONOTONIC
# elif defined(CLOCK_HIGHRES)
# define TIMER_CLOCK CLOCK_HIGHRES
# elif defined(CLOCK_REALTIME)
# define TIMER_CLOCK CLOCK_REALTIME
# else
# error No suitable clock found.  Check docs for clock_gettime.
# endif
#endif


namespace
{
#if !defined(_WIN32) && !defined(__APPLE__)
inline int64_t timespecToNano (const timespec &x)
{
    return (x.tv_sec * 1000000000L) + x.tv_nsec;
}
#endif


// For windows the resolution is in ticks per second, otherwise it is
// already in nanoseconds per sample
Timer::fraction_t timerResolution()
{
#if defined(_WIN32)
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    const int64_t denom = static_cast<int64_t>(freq.QuadPart);
    const Timer::fraction_t resolution(1000000000L, denom);
#elif defined(__APPLE__)
    mach_timebase_info_data_t sTimebaseInfo;
    mach_timebase_info(&sTimebaseInfo);
    const Timer::fraction_t resolution(sTimebaseInfo.numer, sTimebaseInfo.denom);
#else
    timespec tspec;
    clock_getres (TIMER_CLOCK, &tspec);
    const int64_t denom = timespecToNano (tspec);
    const Timer::fraction_t resolution(1L, denom);
#endif
    return resolution;
}
} // namespace


const Timer::fraction_t Timer::timerResolution = ::timerResolution();


double Timer::getTimerResolution()
{
    return Timer::timerResolution * 1e9;
}


Timer::Timer() : m_elapsed(0L), m_time(0L)
{
}


int64_t Timer::getNanoTime()
{
#if defined(_WIN32)
    LARGE_INTEGER perfcount;
    QueryPerformanceCounter(&perfcount);
    // Should never overflow unless dealing with periods over 73 years long
    const int64_t rawtime = static_cast<int64_t>(perfcount.QuadPart);
#elif defined(__APPLE__)
    const int64_t rawtime = static_cast<int64_t>(mach_absolute_time());
#else
    timespec tspec;
    clock_gettime (TIMER_CLOCK, &tspec);
    const int64_t rawtime = timespecToNano (tspec);
#endif
    const int64_t t= Timer::timerResolution * rawtime;
    return t;
}


void Timer::reset()
{
    m_elapsed = 0L;
    m_time    = 0L;
}


void Timer::start()
{
    assert(m_time == 0);
    m_time = getNanoTime();
}


void Timer::stop()
{
    assert(m_time != 0);
    int64_t currtime = getNanoTime();
    m_elapsed += currtime - m_time;
    m_time = 0;
}


// This method may only be used when the timer is stopped
int64_t Timer::nanoTime() const
{
    assert(m_time == 0);
    return m_elapsed;
}


double Timer::milliTime() const
{
    return nanoTime() * 1e-6;
}

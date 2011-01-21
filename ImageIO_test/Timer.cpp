#include "Timer.h"

// Includes for the timer
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif


double Timer::mPeriod = 0.0;

Timer::Timer(bool inHardware) : mHardware(inHardware) {
	if(mPeriod == 0) calibrateCountPeriod(); 
	reset();
}


/* Disable the "unreferenced format parameter" warning */
#if defined(_MSC_VER)
#pragma warning( disable : 4100 )
#endif

void Timer::calibrateCountPeriod(unsigned int inDelay, unsigned int inTimes)
{
#ifdef WIN32
	// use the windows counter
	LARGE_INTEGER lFrequency;
	QueryPerformanceFrequency(&lFrequency);
	mPeriod = 1. / lFrequency.QuadPart;
#else
	if(mHardware) {
#if defined (__GNUG__) && (defined (__i386__) || defined (__ppc__))
		double lPeriod = 0;
		// calibrate by matching the time-stamps with the micro-seconds of gettimeofday
		for(unsigned int i = 0; i < inTimes; ++ i) {
			timeval lStartTime, lTime;
			::gettimeofday(&lStartTime, 0);
			unsigned long long lStartCount = getCount();
			::usleep(inDelay);
			::gettimeofday(&lTime, 0);
			unsigned long long lCount = getCount() - lStartCount;
			lTime.tv_sec -= lStartTime.tv_sec;
			lTime.tv_usec -= lStartTime.tv_usec;
			// dismiss the first run of the loop
			if(i != 0) lPeriod += (lTime.tv_sec + lTime.tv_usec*0.000001)/lCount;
		}
		mPeriod = lPeriod/(inTimes-1);
#else
		// use the microseconds of gettimeofday
		mPeriod = 0.000001;
#endif
	} else {
		// use the microseconds of gettimeofday
		mPeriod = 0.000001;
	}
#endif

}

unsigned long long Timer::getCount(void) const
{
	unsigned long long lCount = 0;
#ifdef WIN32
	LARGE_INTEGER lCurrent;
	QueryPerformanceCounter(&lCurrent);
	lCount = lCurrent.QuadPart;
#else
	if(mHardware) {
#if defined (__GNUG__) && defined (__i386__)
		__asm__ volatile("rdtsc" : "=A" (lCount));
#else
#if defined (__GNUG__) && defined (__ppc__)
		register unsigned int lLow;
		register unsigned int lHigh1;
		register unsigned int lHigh2;
		do {
			// make sure that high bits have not changed
			__asm__ volatile ( "mftbu %0" : "=r" (lHigh1) );
			__asm__ volatile ( "mftb  %0" : "=r" (lLow) );
			__asm__ volatile ( "mftbu %0" : "=r" (lHigh2) );
		} while(lHigh1 != lHigh2);
		// transfer to lCount
		unsigned int *lPtr = (unsigned int*) &lCount;
		*lPtr++ = lHigh1; *lPtr = lLow;
#else
		timeval lCurrent;
		::gettimeofday(&lCurrent, 0);
		lCount = (unsigned long long)lCurrent.tv_sec*1000000 + lCurrent.tv_usec;
#endif
#endif
	} else {
		timeval lCurrent;
		::gettimeofday(&lCurrent, 0);
		lCount = (unsigned long long)lCurrent.tv_sec*1000000 + lCurrent.tv_usec;
	}
#endif
	return lCount;

}

#ifndef PCG_TIMER_H
#define PCG_TIMER_H

// Timer with sub-millisecond accuracy.
// Taken from the Portable Agile C++ Classes (PACC)
// http://manitou.gel.ulaval.ca/~parizeau/PACC
class Timer {

protected:
	bool mHardware; 
	unsigned long long mCount; 
	unsigned long long elapsed;
	static double mPeriod; 

public:
	Timer(bool inHardware=true);

	void calibrateCountPeriod(unsigned int inDelay=10000, unsigned int inTimes=10);

	unsigned long long getCount(void) const;

	double getCountPeriod(void) const {return mPeriod;}

	void reset(void) {mCount = getCount(); elapsed=0;}
	void start(void) {mCount = getCount();}
	void stop(void)  {elapsed += getCount()-mCount;}
	double elapsedSeconds() {return elapsed*mPeriod;} 
};

#endif

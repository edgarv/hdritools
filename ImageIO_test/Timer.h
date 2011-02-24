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

#if defined(_MSC_VER)
#pragma once
#endif
#ifndef PCG_TIMER_H
#define PCG_TIMER_H

#if !defined(_MSC_VER) || _MSC_VER >= 1600
# include <stdint.h>
#else
# if !defined(int64_t)
typedef __int64 int64_t;
# endif
#endif

#include <ctime>


class Timer
{
public:
    Timer();

    void reset();
    void start();
    void stop();
    double milliTime() const;
    int64_t nanoTime() const;

    // Timer resolution in seconds (i.e. 1e-9 for nanosecond accuracy)
    static double getTimerResolution();

    struct fraction_t {
        const int64_t numer;
        const int64_t denom;

        fraction_t() : numer(0L), denom(0L) {}
        fraction_t(int64_t n, int64_t d) : numer(n), denom(d) {}
        fraction_t(const fraction_t & other) : 
        numer(other.numer), denom(other.denom) {}

        int64_t operator*(int64_t n) const {
            return (n * numer) / denom;
        }

        double operator*(double n) const {
            return (n * numer) / denom;
        }
    };

private:
    static int64_t getNanoTime();

    const static fraction_t timerResolution;

    int64_t m_elapsed;
    int64_t m_time;
};

#endif // PCG_TIMER_H

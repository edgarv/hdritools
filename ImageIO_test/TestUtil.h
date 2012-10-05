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

// Helper header with definitions for helper functions for testing
#pragma once
#if !defined(PCG_TESTUTIL_H)
#define PCG_TESTUTIL_H

#include <gtest/gtest.h>
#include <Rgba32F.h>

#include <cmath>

namespace pcg
{

inline bool TestEquals (const pcg::Rgba32F &m, const pcg::Rgba32F &n) {
    return m == n;
}

inline bool floatClose (float a, float b, const int32_t ulp = (1<<5)) {
    union {float f[2]; int32_t i32[2]; } u = {{a, b}};
    return abs(u.i32[0] - u.i32[1]) < ulp;
}

inline bool TestClose (const pcg::Rgba32F &m, const pcg::Rgba32F &n) {
    return floatClose(m.r(), n.r()) &&
           floatClose(m.g(), n.g()) &&
           floatClose(m.b(), n.b()) &&
           floatClose(m.a(), n.a());
}



// Online variance calulation by Knuth, referenced by Wikipedia [August 2012]
//   http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
//   Donald E. Knuth (1998). The Art of Computer Programming, volume 2:
//   Seminumerical Algorithms, 3rd edn., p. 232. Boston: Addison-Wesley.
class VarianceFunctor
{
public:
    VarianceFunctor() : m_n(0), m_mean(0.0), m_M2(0.0),
        m_max(-std::numeric_limits<double>::infinity()) {}

    inline void update(double x) {
        ++m_n;
        double delta = x - m_mean;
        m_mean += delta / m_n;
        m_M2   += delta * (x - m_mean);
        m_max = fmaxd(m_max, x);
    }

    inline size_t n() const {
        return m_n;
    }

    inline double mean() const {
        return m_mean;
    }

    inline double variance() const {
        assert (m_n > 1);
        return m_M2 / (m_n - 1);
    }

    inline double stddev() const {
        return sqrt(variance());
    }

    inline double max() const {
        return m_max;
    }

    void reset() {
        m_n    = 0;
        m_mean = 0.0;
        m_M2   = 0.0;
        m_max  = -std::numeric_limits<double>::infinity();
    }

private:

    inline static double fmaxd(double x, double y) {
        __m128d tmp = _mm_max_sd(_mm_load_sd(&x), _mm_load_sd(&y));
        return _mm_cvtsd_f64(tmp);
    }

    size_t m_n;
    double m_mean;
    double m_M2;
    double m_max;
};

} // namespace pcg


#define ASSERT_RGBA32F_EQ(expected, result) \
    ASSERT_PRED2 (pcg::TestEquals, expected, result)

#define ASSSERT_RGBA32F_CLOSE(expected, result) \
    EXPECT_PRED2 (pcg::TestClose, expected, result)

#endif // PCG_TESTUTIL_H

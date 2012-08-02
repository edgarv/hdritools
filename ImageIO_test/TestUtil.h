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

} // namespace pcg


#define ASSERT_RGBA32F_EQ(expected, result) \
    ASSERT_PRED2 (pcg::TestEquals, expected, result)

#define ASSSERT_RGBA32F_CLOSE(expected, result) \
    EXPECT_PRED2 (pcg::TestClose, expected, result)

#endif // PCG_TESTUTIL_H

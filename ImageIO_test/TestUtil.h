// Helper header with definitions for helper functions for testing
#if defined(_MSC_VER)
#pragma once
#endif
#if !defined(PCG_TESTUTIL_H)
#define PCG_TESTUTIL_H

#include <gtest/gtest.h>
#include <Rgba32F.h>

namespace
{

bool TestEquals (const pcg::Rgba32F &m, const pcg::Rgba32F &n) {
    return m == n;
}
} // namespace

#if !defined(_MSC_VER) || defined(_M_X64)
#define ASSERT_RGBA32F_EQ(expected, result) \
    ASSERT_PRED2 (::TestEquals, expected, result)
#else
#define ASSERT_RGBA32F_EQ(expected, result)  {         \
    ASSERT_FLOAT_EQ((expected).r(), (result).r());     \
    ASSERT_FLOAT_EQ((expected).g(), (result).g());     \
    ASSERT_FLOAT_EQ((expected).b(), (result).b());     \
    ASSERT_FLOAT_EQ((expected).a(), (result).a());     \
    }
#endif


#endif // PCG_TESTUTIL_H

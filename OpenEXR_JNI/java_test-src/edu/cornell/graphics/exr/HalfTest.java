/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/
package edu.cornell.graphics.exr;

import jp.ac.hiroshima_u.sci.math.saito.tinymt.TinyMT32;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;



public class HalfTest {
    
    // Common random number generator
    private static TinyMT32 base_random;
    
    // Private copy for each test, so that they have the same state
    private TinyMT32 rnd;
    
    public HalfTest() {
    }

    @BeforeClass
    public static void setUpClass() throws Exception {
        // Initialize the random generator
        base_random = TinyMT32.getDefault(new int[]{
            0xf8d37386, 0xbbaaa3c3, 0xb283959f, 0xe5bb1ad5, 0x0d205fe1,
            0xe3b75001, 0xdc6435f4, 0x3d4aaa7e, 0xa8316aa1, 0x7ddbb3f2,
            0x977595dd, 0x64bc315e, 0xf6e1d0fe, 0x76f3524b, 0xd23d1547,
            0x8808cb08, 0xa929cdca, 0xae88599a, 0x6b399a10, 0x672c76a6,
            0x414ec77e, 0x218f7d36, 0xdb4b04f1, 0x7a84898d, 0xc830f6d4,
            0x425fe2d1, 0xfcd6069e, 0x61e63908, 0xfc86278b, 0x1f95294e
        });
    }
    
    @Before
    public void setUp() {
        if (base_random == null) {
            throw new IllegalStateException("Null base random generator.");
        }
        rnd = base_random.cloneRandom(); 
    }
    
    
    // Verbatim process from Half.shortBitsToFloat(short)
    private static float evalHalfBits(short bits) {
        
        int s = (bits >> 15) == 0 ? 1 : -1;
        int e = (bits >> 10) & 0x1f;
        int m = (e == 0) ?
                        (bits & 0x3ff) << 1 :
                        (bits & 0x3ff) | 0x400;
        
        // Zero
        if (e == 0 && m == 0) {
            return s * 0.0f;
        }
        
        // NaN or infinity
        if (e == 31) {
            if (m == 0) {
                return s * Float.POSITIVE_INFINITY;
            }
            else {
                return Float.NaN;
            }
        }
        
        // 2^(e-25)
        double v = e >= 25 ? (1 << (e-25)) : 1.0 / (1 << (25-e));
        return (float) ((s * m) * v);
    }
    


    /**
     * Test of floatToShortBits method, of class Half.
     */
    @Test
    public void testFloatToShortBits() {
        System.out.println("floatToShortBits");
        
        // Special values
        assertEquals((short)0x0000, Half.floatToShortBits( 0.0f));
        assertEquals((short)0x8000, Half.floatToShortBits(-0.0f));
        assertEquals(Half.POSITIVE_INFINITY_BITS,
                Half.floatToShortBits(Float.POSITIVE_INFINITY));
        assertEquals(Half.NEGATIVE_INFINITY_BITS,
                Half.floatToShortBits(Float.NEGATIVE_INFINITY));
        assertEquals(Half.NaN_BITS, Half.floatToShortBits(Float.NaN));
        
        assertEquals((short)0x0001, Half.floatToShortBits(Half.MIN_VALUE));
        assertEquals((short)0x0400, Half.floatToShortBits(Half.MIN_NORMAL));
        {
            // Biggest denormal, (1.0/(1<<14))*(1.0/(1<<10)*0x3ff) ~= 6.09756e−5
            float f = Float.intBitsToFloat(0x387fc000);
            assertEquals((short)0x03ff, Half.floatToShortBits(f));
            
            // Maximum value which can be rounded to a half
            f = Float.intBitsToFloat(0x477fefff);
            assertEquals((short) 0x7bff, Half.floatToShortBits(f));
            
            // First overflow
            f = Float.intBitsToFloat(0x477ff000);
            assertEquals(Half.POSITIVE_INFINITY_BITS, Half.floatToShortBits(f));
        }
        
        // Test exactly-representable integers
        for (float value = -2048.0f; value <= 2048.0f; value += 1.0f) {
            short bits = Half.floatToShortBits(value);
            float result = evalHalfBits(bits);
            assertTrue(value == result);
        }
        
        // Test a large set of representable value
        for (int i = 0; i < 1000000; ++i) {
            float absValue = (float) (rnd.nextDouble() * 
                    (rnd.nextFloat()>0.9f ? Half.MAX_VALUE : Half.MIN_NORMAL));
            float value = absValue * (rnd.nextBoolean() ? 1.0f : -1.0f);
            short bits = Half.floatToShortBits(value);
            float result = evalHalfBits(bits);
            if (absValue != 0) {
                if (absValue >= Half.MIN_NORMAL) {
                    assertTrue(Math.abs((result - value))/value <= 9.8e-4);
                }
                else {
                    assertEquals(result, value, 6.0e-8);
                }
            }
            else {
                assertEquals(value, result, 0.0);
            }
        }
        
        // Invalid values: those greater than or equal to 65520.0f
        // (IEEE-754 bit pattern: 0x477ff000). Test patterns in the range
        // [0x477ff000, 0x7fffffff] inclusive with both signs.
        for (int i = 0; i < 1000000; ++i) {
            int floatBits = 0x477ff000 + rnd.nextInt(947916800);
            assert(0x477ff000 <= floatBits && floatBits <= 0x7fffffff);
            floatBits |= rnd.nextBoolean() ? 0x0 : 0x80000000;
            float value = Float.intBitsToFloat(floatBits);
            short halfBits = Half.floatToShortBits(value);
            
            if (!Float.isNaN(value)) {
                // All conversions overflow
                if (value > 0.0f) {
                    assertEquals(Half.POSITIVE_INFINITY_BITS, halfBits);
                } else {
                    assertEquals(Half.NEGATIVE_INFINITY_BITS, halfBits);
                }
            }
            else {
                // There are several bit patterns for NaN
                int e = halfBits >> 10 & 0x1f;
                int m = halfBits & 0x3ff;
                assertEquals(e, 0x001f);
                assertTrue(m != 0);
            }
        }
    }

    /**
     * Test of shortBitsToFloat method, of class Half.
     */
    @Test
    public void testShortBitsToFloat() {
        System.out.println("shortBitsToFloat");
        
        // Special values
        assertEquals( 0.0f, Half.shortBitsToFloat((short)0x0000), 0.0);
        assertEquals(-0.0f, Half.shortBitsToFloat((short)0x8000), 0.0);
        assertEquals(Float.POSITIVE_INFINITY,
                Half.shortBitsToFloat(Half.POSITIVE_INFINITY_BITS), 0.0);
        assertEquals(Float.NEGATIVE_INFINITY,
                Half.shortBitsToFloat(Half.NEGATIVE_INFINITY_BITS), 0.0);
        assertTrue(Float.isNaN(Half.shortBitsToFloat(Half.NaN_BITS)));
        assertEquals(Half.MIN_VALUE,  Half.shortBitsToFloat((short)0x0001),0.0);
        assertEquals(Half.MIN_NORMAL, Half.shortBitsToFloat((short)0x0400),0.0);
        assertEquals(Half.MAX_VALUE,  Half.shortBitsToFloat((short)0x7bff),0.0);
        {
            // Biggest denormal, (1.0/(1<<14))*(1.0/(1<<10)*0x3ff) ~= 6.09756e−5
            final float f = Float.intBitsToFloat(0x387fc000);
            assertEquals(f, Half.shortBitsToFloat((short)0x03ff), 0.0);
        }
        
        // Denormals & normals
        for (int i = 0x0001; i <= 0x7bff; ++i) {
            // Positive
            short bits = (short) i;
            float expected = evalHalfBits(bits);
            assertEquals(expected, Half.shortBitsToFloat(bits), 0.0);
            
            // Negative
            bits |= 0x8000;
            expected *= -1;
            assertEquals(expected, Half.shortBitsToFloat(bits), 0.0);
        }
        
        // NaN
        for (int i = 0x7c01; i <= 0x7fff; ++i) {
            short bits = (short) i;
            float result = Half.shortBitsToFloat(bits);
            assertTrue(Float.isNaN(result));
            
            bits |= 0x8000;
            result = Half.shortBitsToFloat(bits);
            assertTrue(Float.isNaN(result));
        }
    }
}

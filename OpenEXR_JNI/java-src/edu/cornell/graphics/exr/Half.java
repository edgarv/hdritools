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

/**
 * Utility methods to convert from/to single precision floating point values to
 * the binary representation of the <code>half</code> type.
 */
public final class Half {
    
    /**
     * A constant holding the smallest positive normal value of type
     * {@code half}, 2<sup>-14</sup>.
     */
    public final static float MIN_NORMAL = 6.10351562e-05f;
    
    /**
     * A constant holding the smallest positive nonzero value of type
     * {@code half}, 2<sup>-24</sup>.
     */
    public final static float MIN_VALUE = 5.96046448e-08f;
    
    /**
     * A constant holding the largest positive finite value of type
     * {@code half}, (2-2<sup>-10</sup>)&middot;2<sup>15</sup>.
     */
    public final static float MAX_VALUE = 65504.0f;
    
    /** Minimum exponent a normalized {@code half} variable may have. */
    public final static int MIN_EXPONENT = -13;
    
    /** Maximum exponent a finite {@code half} variable may have. */
    public final static int MAX_EXPONENT = 16;
    
    /**
     * A constant holding the smallest positive {@code e} for which
     * {@code half(1.0 + e) != half(1.0)}.
     */
    public final static float EPSILON = 0.00097656f;
    
    /**
     * A constant holding the bit pattern of a
     * Not-a-Number (NaN) value of type {@code half}.
     */
    public final static short NaN_BITS = 0x7e00;
    
    /**
     * A constant holding the bit pattern of the negative infinity
     * of type <code>half</code>
     */
    public final static short NEGATIVE_INFINITY_BITS = (short) 0xfc00;
    
    /**
     * A constant holding the bit pattern of the positive infinity
     * of type <code>half</code>
     */
    public final static short POSITIVE_INFINITY_BITS = 0x7c00;
    
    /** The number of bits used to represent a {@code half} value. */
    public final static int SIZE = 16;
    
    /**
     * <p>Returns a representation of the specified floating-point value
     * according to the IEEE 754-2008 &quot;half format&quot; bit layout.</p>
     * 
     * <p>Bit 15 (the bit that is selected by the mask {@code 0x8000}
     * represents the sign of the floating point number. Bits 14-10 (the bits
     * that are selected by the mask {@code 0x7c00} represents the
     * exponent. Bits 9-0 (the bits that are selected by the mask
     * {@code 0x3ff}) represent the significand (sometimes called the
     * mantissa) of the floating-point number.
     * 
     * <p>If the argument is positive infinity, the result is {@code 0x7c00}.
     * 
     * <p>If the argument is negative infinity, the result is {@code 0xfc00}.
     * 
     * <p>If the argument is NaN, the result is {@code 0x7e00}.
     * 
     * <p>In all cases, the result is an integer that, when given to the
     * {@link #shortBitsToFloat(short) } method, will produce a floating-point
     * value the same as the argument to <code>floatToShortBits</code> (except
     * all NaN values are collapsed to a single &quot;canonical&quot;
     * NaN value).
     * 
     * @param value a floating-point number.
     * @return the bits that represent the half-precision floating-point number.
     */
    public static short floatToShortBits(float value) {
        // Method from public domain SSE2-friendly code by Fabian "ryg" Giesen
        //   https://gist.github.com/2156668 [August 2012]
        //
        
        final int f32infty_u = (255 << 23);
        final int f16infty_u = ( 31 << 23);
        final float magic = Float.intBitsToFloat(15<<23); // 2^(15-127)
        final int sign_mask  = 0x80000000;
        final int round_mask = ~0xfff;
        
        int f_u = Float.floatToRawIntBits(value);
        short o_u;
        
        final int sign = f_u & sign_mask;
        f_u ^= sign;
        
        // All signed int compares are safe since all operands are
        // bellow 0x80000000.
        
        if (f_u >= f32infty_u) { // Inf or NaN (all exponent bits set)
            //NaN->qNaN and Inf->Inf
            o_u = (f_u > f32infty_u) ? (short) 0x7e00 : (short) 0x7c00;
        }
        else { // (De)normalized number or zero
            f_u &= round_mask;
            float f_f = Float.intBitsToFloat(f_u);
            f_f *= magic;
            f_u = Float.floatToRawIntBits(f_f);
            f_u -= round_mask;
            // Clamp to signed infinity if overflowed
            if (f_u > f16infty_u) f_u = f16infty_u;
            
            o_u = (short) (f_u >> 13); // Take the bits!
        }
        
        o_u |= sign >> 16;        
        return o_u;
    }
    
    
    
    /**
     * Returns the {@code float} value equivalent to a given half-precision
     * bit representation.
     * The argument is considered to be a representation of a
     * floating-point value according to the IEEE 754-2008 floating-point
     * "half format" bit layout.
     *
     * <p>If the argument is {@code 0x7c00}, the result is positive infinity.
     *
     * <p>If the argument is {@code 0xfc00}, the result is negative infinity.
     *
     * <p>If the argument is any value in the range
     * {@code 0x7c01} through {@code 0x7fff} or in
     * the range {@code 0xfc01} through
     * {@code 0xffff}, the result is a NaN.  No IEEE 754
     * floating-point operation provided by Java can distinguish
     * between two NaN values of the same type with different bit
     * patterns.  Distinct values of NaN are only distinguishable by
     * use of the {@code Float.floatToRawIntBits} method.
     *
     * <p>In all other cases, let <i>s</i>, <i>e</i>, and <i>m</i> be three
     * values that can be computed from the argument:
     *
     * <blockquote><pre>
     * int s = ((bits &gt;&gt; 15) == 0) ? 1 : -1;
     * int e = ((bits &gt;&gt; 10) & 0x1f);
     * int m = (e == 0) ?
     *                 (bits & 0x3ff) &lt;&lt; 1 :
     *                 (bits & 0x3ff) | 0x400;
     * </pre></blockquote>
     *
     * Then the floating-point result equals the value of the mathematical
     * expression <i>s</i>&middot;<i>m</i>&middot;2<sup><i>e</i>-25</sup>.
     *
     * <p>Note that this method may not be able to return a
     * {@code float} NaN with exactly same bit pattern as the
     * {@code short} argument.  IEEE 754 distinguishes between two
     * kinds of NaNs, quiet NaNs and <i>signaling NaNs</i>.  The
     * differences between the two kinds of NaN are generally not
     * visible in Java.  Arithmetic operations on signaling NaNs turn
     * them into quiet NaNs with a different, but often similar, bit
     * pattern.  However, on some processors merely copying a
     * signaling NaN also performs that conversion.  In particular,
     * copying a signaling NaN to return it to the calling method may
     * perform this conversion.  So {@code shortBitsToFloat} may
     * not be able to return a {@code float} with a signaling NaN
     * bit pattern.  Consequently, for some {@code short} values,
     * {@code floatToShortBits(shortBitsToFloat(start))} may
     * <i>not</i> equal {@code start}.  Moreover, which
     * particular bit patterns represent signaling NaNs is platform
     * dependent; although all NaN bit patterns, quiet or signaling,
     * must be in the NaN range identified above.
     *
     * @param   bits   a short.
     * @return  the {@code float} floating-point value with the equivalent bit
     *          pattern.
     */
    public static float shortBitsToFloat(short bits) {
        // Method from public domain code by Fabian "ryg" Giesen
        //   https://gist.github.com/2156668 [August 2012]
        //
        final float magic = Float.intBitsToFloat(113 << 23); // 2^-14
        final int shifted_exp = 0x7c00 << 13; // exponent mask after shift
        int o_u;
        
        o_u = (bits & 0x7fff) << 13;    // exponent/mantissa bits
        int exp = shifted_exp & o_u;    // just the exponent
        o_u += (127 - 15) << 23;        // exponent adjust
        
        // handle exponent special cases
        if (exp == shifted_exp) { // Inf/NaN?
            o_u += (128 - 16) << 23;    // extra exp adjust
        } else if (exp == 0) {    // Zero/Denormal?
            o_u += 1 << 23;             // extra exp adjust
            float f = Float.intBitsToFloat(o_u) - magic; // renormalize
            o_u = Float.floatToRawIntBits(f);
        }
        
        o_u |= (bits & 0x8000) << 16;   // sign bit
        float f = Float.intBitsToFloat(o_u);
        return f;
    }
}

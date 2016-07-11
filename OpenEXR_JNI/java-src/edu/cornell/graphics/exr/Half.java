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
 * <p>The {@code Half} class wraps the bits of an IEEE 754-2008 half-precision
 * floating point value in an object. An object of type {@code Half} contains
 * a single field whose type is {@code short} (the bits of the actual floating
 * point value.)</p>
 * 
 * <p>In addition, this class provides several methods for converting a
 * {@code Half} to a {@code float}, a {@code float} to a {@code Half}, a
 * {@code Half} to a {@code String} and a {@code String} to a {@code Half};
 * as well as other constants and methods useful when dealing with a
 * {@code Half}.</p>
 * 
 * <p>It also provides methods for conversions between a {@code short}
 * containing the bit pattern of a half-precision floating point value and the
 * native {@code float} type.</p>
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class Half extends Number implements Comparable<Half> {
    
    private static final long serialVersionUID = 4777709089414412809L;
    
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
     * A constant holding the value of Not-a-Number (NaN) of type {@code half}.
     */
    public final static Half NaN = new Half(NaN_BITS);
    
    /**
     * A constant holding the bit pattern of the negative infinity
     * of type <code>half</code>
     */
    public final static short NEGATIVE_INFINITY_BITS = (short) 0xfc00;
    
    /**
     * A constant holding the value of the negative infinity
     * of type {@code half}.
     */
    public final static Half NEGATIVE_INFINITY =
            new Half(Half.NEGATIVE_INFINITY_BITS);
    
    /**
     * A constant holding the bit pattern of the positive infinity
     * of type <code>half</code>
     */
    public final static short POSITIVE_INFINITY_BITS = 0x7c00;
    
    /**
     * A constant holding the value of the positive infinity
     * of type {@code half}.
     */
    public final static Half POSITIVE_INFINITY =
            new Half(Half.POSITIVE_INFINITY_BITS);
    
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
            if (f_u > f16infty_u) {
                f_u = f16infty_u;
            }
            
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
     * int e = ((bits &gt;&gt; 10) &amp; 0x1f);
     * int m = (e == 0) ?
     *                 (bits &amp; 0x3ff) &lt;&lt; 1 :
     *                 (bits &amp; 0x3ff) | 0x400;
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
    
    /**
     * Returns {@code true} if the specified bit pattern corresponding to a
     * half-precision floating point number is a
     * Not-a-Number (NaN) value, {@code false} otherwise.
     *
     * @param bits the bit pattern to be tested.
     * @return  {@code true} if the argument is NaN;
     *          {@code false} otherwise.
     */
    public static boolean isNaN(short bits) {
        return ((bits & 0x7c00) == 0x7c00 && (bits & 0x03ff) != 0);
    }
    
    /**
     * Returns {@code true} if the specified bit pattern corresponding to a
     * half-precision floating point number is infinitely
     * large in magnitude, {@code false} otherwise.
     *
     * @param bits the bit pattern to be tested.
     * @return  {@code true} if the argument is positive infinity or
     *          negative infinity; {@code false} otherwise.
     */
    public static boolean isInfinite(short bits) {
        return bits == POSITIVE_INFINITY_BITS || bits == NEGATIVE_INFINITY_BITS;
    }
    
    
    /**
     * The bit pattern of the Half.
     *
     * @serial
     */
    private final short bits;
    
    /**
     * Constructs a newly allocated {@code Half} object with the bit pattern
     * of the argument.
     * 
     * @param bits the bit pattern which represents the {@code Half}
     */
    public Half(short bits) {
        this.bits = bits;
    }
    
    /**
     * Constructs a newly allocated {@code Half} object that
     * represents the argument converted to type {@code half}.
     *
     * @param value the value to be represented by the {@code Half}.
     * @see #floatToShortBits(float) 
     */
    public Half(float value) {
        this.bits = floatToShortBits(value);
    }

    /**
     * Constructs a newly allocated {@code Half} object that
     * represents the argument converted to type {@code half}.
     *
     * @param value the value to be represented by the {@code Half}.
     * @see #floatToShortBits(float) 
     */
    public Half(double value) {
        this.bits = floatToShortBits((float) value);
    }

    /**
     * Constructs a newly allocated {@code Half} object that
     * represents the half-precision floating-point value
     * represented by the string. The string is converted to a
     * {@code float} value as if by the {@code valueOf} method.
     *
     * @param s a string to be converted to a {@code Half}.
     * @throws NumberFormatException if the string does not contain a
     *               parsable number.
     * @see java.lang.Float#valueOf(java.lang.String)
     * @see #floatToShortBits(float) 
     */
    public Half(String s) throws NumberFormatException {
        // REMIND: this is inefficient
        this.bits = floatToShortBits(Float.valueOf(s).floatValue());
    }
    
    /**
     * <p>Returns a representation of the calling object
     * according to the IEEE 754-2008 floating-point "half format" bit
     * layout, preserving Not-a-Number (NaN) values.</p>
     * 
     * <p>Bit 15 (the bit that is selected by the mask {@code 0x8000}
     * represents the sign of the floating point number. Bits 14-10 (the bits
     * that are selected by the mask {@code 0x7c00} represents the
     * exponent. Bits 9-0 (the bits that are selected by the mask
     * {@code 0x3ff}) represent the significand (sometimes called the
     * mantissa) of the floating-point number.</p>
     * 
     * <p>If the argument is positive infinity, the result is {@code 0x7c00}.
     * 
     * <p>If the argument is negative infinity, the result is {@code 0xfc00}.
     * 
     * <p>If the argument is NaN, the result is the short representing
     * the actual NaN value.  Unlike the {@code halfToShortBits}
     * method, {@code halfToRawShortBits} does not collapse all the
     * bit patterns encoding a NaN to a single &quot;canonical&quot;
     * NaN value.</p>
     * 
     * <p>In all cases, the result is a short that, when given to the
     * {@link #shortBitsToFloat(short) } method, will produce a
     * floating-point value the same as invoking object.
     * 
     * @return the bits that represent the half-precision floating-point number.
     */
    public short halfToRawShortBits() {
        return bits;
    }
    
    /**
     * <p>Returns a representation of the calling object
     * according to the IEEE 754-2008 floating-point "half format" bit
     * layout.</p>
     * 
     * <p>Bit 15 (the bit that is selected by the mask {@code 0x8000}
     * represents the sign of the floating point number. Bits 14-10 (the bits
     * that are selected by the mask {@code 0x7c00} represents the
     * exponent. Bits 9-0 (the bits that are selected by the mask
     * {@code 0x3ff}) represent the significand (sometimes called the
     * mantissa) of the floating-point number.</p>
     * 
     * <p>If the argument is positive infinity, the result is {@code 0x7c00}.
     * </p>
     * <p>If the argument is negative infinity, the result is {@code 0xfc00}.
     * </p>
     * <p>If the argument is NaN, the result is {@code 0x7e00}.</p>
     * 
     * <p>In all cases, the result is a short that, when given to the
     * {@link #shortBitsToFloat(short) } method, will produce a
     * floating-point value the same as invoking object.</p>
     * 
     * @return the bits that represent the half-precision floating-point number.
     */
    public short halfToShortBits() {
        return Half.halfToShortBits(this.bits);
    }
    
    /**
     * <p>Returns a representation of the given bit pattern adhering to
     * the IEEE 754-2008 floating-point "half format" layout.</p>
     * 
     * <p>Bit 15 (the bit that is selected by the mask {@code 0x8000}
     * represents the sign of the floating point number. Bits 14-10 (the bits
     * that are selected by the mask {@code 0x7c00} represents the
     * exponent. Bits 9-0 (the bits that are selected by the mask
     * {@code 0x3ff}) represent the significand (sometimes called the
     * mantissa) of the floating-point number.</p>
     * 
     * <p>If the argument is positive infinity, the result is {@code 0x7c00}.
     * </p>
     * <p>If the argument is negative infinity, the result is {@code 0xfc00}.
     * </p>
     * <p>If the argument is NaN, the result is {@code 0x7e00}.</p>
     * 
     * <p>In all cases, the result is a short that, when given to the
     * {@link #shortBitsToFloat(short) } method, will produce a
     * floating-point value the same as invoking object.</p>
     * 
     * @param halfBits the bit pattern of a {@code half} floating-point number.
     * @return the bits that represent the half-precision floating-point number,
     *         unifying all NaN values.
     */
    public static short halfToShortBits(short halfBits) {
        // Check for NaN and return canonical value
        short result = halfBits;
        if ((result & 0x7c00) == 0x7c00 && (result & 0x03ff) != 0) {
            result = Half.NaN_BITS;
        }
        return result;
    }
    
    /**
     * Returns {@code true} if this {@code Half} value is a
     * Not-a-Number (NaN), {@code false} otherwise.
     *
     * @return  {@code true} if the value represented by this object is
     *          NaN; {@code false} otherwise.
     */
    public boolean isNaN() {
        return isNaN(this.bits);
    }
    
    /**
     * Returns {@code true} if this {@code Half} value is
     * infinitely large in magnitude, {@code false} otherwise.
     *
     * @return  {@code true} if the value represented by this object is
     *          positive infinity or negative infinity;
     *          {@code false} otherwise.
     */
    public boolean isInfinite() {
        return isInfinite(this.bits);
    }
    
    /**
     * Returns the value of this {@code Half} as a
     * {@code byte} (by casting its {@code float} value to a {@code byte}).
     *
     * @return  the {@code float} value represented by this object
     *          converted to type {@code byte}
     * @see #floatValue() 
     */
    @Override
    public byte byteValue() {
        return (byte) shortBitsToFloat(bits);
    }

    /**
     * Returns the value of this {@code Half} as a
     * {@code short} (by casting its {@code float} value to a {@code short}).
     *
     * @return  the {@code float} value represented by this object
     *          converted to type {@code short}
     * @see #floatValue() 
     */
    @Override
    public short shortValue() {
        return (short) shortBitsToFloat(bits);
    }

    /**
     * Returns the value of this {@code Half} as an
     * {@code int} (by casting its {@code float} value to an {@code int}).
     *
     * @return  the {@code float} value represented by this object
     *          converted to type {@code int}
     * @see #floatValue() 
     */
    @Override
    public int intValue() {
        return (int) shortBitsToFloat(bits);
    }

    /**
     * Returns the value of this {@code Half} as a
     * {@code long} (by casting its {@code float} value to a {@code long}).
     *
     * @return  the {@code float} value represented by this object
     *          converted to type {@code long}
     * @see #floatValue() 
     */
    @Override
    public long longValue() {
        return (long) shortBitsToFloat(bits);
    }

    /**
     * Returns the exact value of this {@code Half} as a
     * {@code double} by converting its bit pattern as per the
     * {@link #shortBitsToFloat(short) } method.
     * 
     * @return the {@code float} value represented by this object.
     * @see #shortBitsToFloat(short) 
     */
    @Override
    public float floatValue() {
        return shortBitsToFloat(bits);
    }

    /**
     * Returns the exact value of this {@code Half} as a
     * {@code double} (by casting its {@code float} value to an {@code double}).
     *
     * @return  the {@code float} value represented by this object
     *          converted to type {@code double}
     * @see #floatValue() 
     */
    @Override
    public double doubleValue() {
        return (double) shortBitsToFloat(bits);
    }
    
    /**
     * Compares this object against the specified object.  The result
     * is {@code true} if and only if the argument is not
     * {@code null} and is a {@code Half} object that
     * represents a {@code half} with the same value as the
     * {@code half} represented by this object. For this
     * purpose, two {@code half} values are considered to be the
     * same if and only if the method {@link #halfToShortBits() }
     * returns the identical {@code short} value when applied to each.
     * <p>
     * Note that in most cases, for two instances of class
     * {@code Half}, {@code h1} and {@code h2}, the value
     * of {@code h1.equals(h2)} is {@code true} if and only if
     * <blockquote><pre>
     *   h1.floatValue() == h2.floatValue()
     * </pre></blockquote>
     * <p>
     * also has the value {@code true}. However, there are two exceptions:
     * <ul>
     * <li>If {@code h1} and {@code h2} both represent
     *     {@code Half.NaN}, then the {@code equals} method returns
     *     {@code true}, even though the operator {@code ==} applied to the
     *     numerical values has the value {@code false}.
     * <li>If {@code h1} represents {@code +0.0f} while
     *     {@code h2} represents {@code -0.0f}, or vice
     *     versa, the {@code equal} test has the value
     *     {@code false}, even though {@code 0.0f==-0.0f}
     *     has the value {@code true}.
     * </ul>
     * This definition allows hash tables to operate properly.
     *
     * @param obj the object to be compared
     * @return {@code true} if the objects are the same;
     *         {@code false} otherwise.
     * @see #halfToShortBits() 
     */
    @Override
    public boolean equals(Object obj) {
        return (obj instanceof Half)
                && (((Half)obj).halfToShortBits() == halfToShortBits());
    }

    /**
     * Returns a hash code for this {@code Half} object. The
     * result is the integer bit representation, exactly as produced
     * by the method {@link #halfToRawShortBits() }.
     *
     * @return a hash code value for this object.  
     */
    @Override
    public int hashCode() {
        return (int) bits;
    }

    @Override
    public String toString() {
        return Float.toString(shortBitsToFloat(bits));
    }
    
    public String toHexString() {
        return Float.toHexString(shortBitsToFloat(bits));
    }

    /**
     * Compares two {@code Half} objects numerically.  There are
     * two ways in which comparisons performed by this method differ
     * from those performed by the Java language numerical comparison
     * operators ({@code &lt;, &lt;=, ==, &gt;= &gt;}) when
     * applied to primitive {@code float} values:
     * <ul><li>
     *		{@code Half.NaN} is considered by this method to
     *		be equal to itself and greater than all other
     *		{@code float} values
     *		(including {@code Half.POSITIVE_INFINITY}).
     * <li>
     *		{@code 0.0f} is considered by this method to be greater
     *		than {@code -0.0f}.
     * </ul>
     * This ensures that the <i>natural ordering</i> of <tt>Half</tt>
     * objects imposed by this method is <i>consistent with {@code equals}</i>.
     *
     * @param anotherHalf   the {@code Half} to be compared.
     * @return  the value {@code 0} if {@code anotherHalf} is
     *		numerically equal to this {@code Half}; a value
     *		less than {@code 0} if this {@code Half}
     *		is numerically less than {@code anotherHalf};
     *		and a value greater than {@code 0} if this
     *		{@code Half} is numerically greater than
     *		{@code anotherHalf}.
     *		
     * @see Comparable#compareTo(Object)
     */
    @Override
    public int compareTo(Half anotherHalf) {
        return Half.compare(this.bits, anotherHalf.bits);
    }
    
    /**
     * Compares the two specified {@code short} bit patterns each representing
     * a half-precision floating-point value. The sign
     * of the integer value returned is the same as that of the
     * integer that would be returned by the call:
     * {@code 
     * <pre>
     *    new Half(bitsH1).compareTo(new Half(bitsH2))
     * </pre>}
     *
     * @param bitsH1 the bit pattern of the first {@code half} to compare.
     * @param bitsH2 the bit pattern of the first {@code half} to compare.
     * @return  the value {@code 0} if the {@code half} represented by
     *          {@code bitsH1} is numerically equal to the {@code half}
     *          represented by {@code bitsH2}; a value less than {@code 0}
     *          if the {@code half} represented by {@code bitsH1} is numerically
     *          less than {@code half} represented by {@code bitsH2}; and a
     *          value greater than {@code 0} if the {@code half} represented by
     *          {@code bitsH1} is numerically greater than the {@code half}
     *          represented by {@code bitsH2}.
     */
    public static int compare(short bitsH1, short bitsH2) {
        if (!Half.isNaN(bitsH1) && !Half.isNaN(bitsH2) &&
            (((bitsH1 & 0x7fff) != 0) || (bitsH2 & 0x7fff) != 0)) {
            // Change from sign-magnitude to two-complement:
            // http://www.cygnus-software.com/papers/comparingfloats/Comparing%20floating%20point%20numbers.htm
            
            int a = (bitsH1 & 0x8000) == 0 ? bitsH1 : (short)0x8000 - bitsH1;
            int b = (bitsH2 & 0x8000) == 0 ? bitsH2 : (short)0x8000 - bitsH2;
            int result = a - b;
            return result;
        }
        
        // At least one NaN or two zeros; unify all NaN
        bitsH1 = Half.halfToShortBits(bitsH1);
        bitsH2 = Half.halfToShortBits(bitsH2);
        
        return (bitsH1 == bitsH2 ?  0 : // Values are equal
                (bitsH1 < bitsH2 ? -1 : // (-0.0, 0.0) or (!NaN, NaN)
                 1));
    }
}

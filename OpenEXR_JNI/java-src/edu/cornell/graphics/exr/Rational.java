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
 * Rational number represented as a numerator ({@code n}) and a denominator
 * ({@code d}). Note that the denominator is considered to hold the bit pattern
 * of an <tt>unsigned integer</tt>, thus if <code>d &lt; 0</code> its actual
 * value is {@code (0x100000000L + d)}.
 */
public class Rational {
    
    /** Numerator */
    public int n;
    
    /** 
     * Denominator, with valid values in the closed interval
     * [0, 2<sup>32</sup>-1]: the value is interpreted as an unsigned integer.
     */
    public int d;
    
    public Rational() {}
    
    public Rational(Rational other) {
        this.n = other.n;
        this.d = other.d;
    }

    public Rational(int n, int d) {
        this.n = n;
        this.d = d;
    }
    
    /**
     * Returns the value of the denominator interpreted as an unsigned
     * integer value with range [0, 2<sup>32</sup>-1].
     * 
     * @return the denominator interpreted as an unsigned integer.
     */
    public long getUnsignedDenominator() {
        return d >= 0 ? (long)d : 0x100000000L + d;
    }
    
    public double doubleValue() {
        return (double)n / (double)getUnsignedDenominator();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Rational other = (Rational) obj;
        if (this.n != other.n) {
            return false;
        }
        if (this.d != other.d) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 23 * hash + this.n;
        hash = 23 * hash + this.d;
        return hash;
    }

    @Override
    public String toString() {
        return Integer.toString(n) + '/' + getUnsignedDenominator();
    }
    
}

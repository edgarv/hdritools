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

package edu.cornell.graphics.exr.ilmbaseto;

/**
 * Transfer object for a 3x3 matrix corresponding to
 * {@code Imath::Matrix33<class T>>} in the IlmBase C++ library. The matrix
 * organization is:
 * <pre>
 *     m00 m01 m02
 *     m10 m11 m12
 *     m20 m21 m22
 * </pre>
 */
public class Matrix33<T extends Number> {
    public T m00, m01, m02;
    public T m10, m11, m12;
    public T m20, m21, m22;
    
    public Matrix33() {}
    
    public Matrix33(T a) {
        m00 = m01 = m02 = a;
        m10 = m11 = m12 = a;
        m20 = m21 = m22 = a;
    }
    
    public Matrix33(Matrix33<T> other) {
        this.m00 = other.m00;
        this.m01 = other.m01;
        this.m02 = other.m02;
        
        this.m10 = other.m10;
        this.m11 = other.m11;
        this.m12 = other.m12;
        
        this.m20 = other.m20;
        this.m21 = other.m21;
        this.m22 = other.m22;
    }
    
    public Matrix33(T m00, T m01, T m02,
        T m10, T m11, T m12,
        T m20, T m21, T m22) {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        
        this.m10 = m10;
        this.m11 = m11;
        this.m12 = m12;
        
        this.m20 = m20;
        this.m21 = m21;
        this.m22 = m22;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Matrix33<T> other = (Matrix33<T>) obj;
        if (this.m00 != other.m00 &&
                (this.m00 == null || !this.m00.equals(other.m00))) {
            return false;
        }
        if (this.m01 != other.m01 &&
                (this.m01 == null || !this.m01.equals(other.m01))) {
            return false;
        }
        if (this.m02 != other.m02 &&
                (this.m02 == null || !this.m02.equals(other.m02))) {
            return false;
        }
        if (this.m10 != other.m10 &&
                (this.m10 == null || !this.m10.equals(other.m10))) {
            return false;
        }
        if (this.m11 != other.m11 &&
                (this.m11 == null || !this.m11.equals(other.m11))) {
            return false;
        }
        if (this.m12 != other.m12 &&
                (this.m12 == null || !this.m12.equals(other.m12))) {
            return false;
        }
        if (this.m20 != other.m20 &&
                (this.m20 == null || !this.m20.equals(other.m20))) {
            return false;
        }
        if (this.m21 != other.m21 &&
                (this.m21 == null || !this.m21.equals(other.m21))) {
            return false;
        }
        if (this.m22 != other.m22 &&
                (this.m22 == null || !this.m22.equals(other.m22))) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 31 * hash + (this.m00 != null ? this.m00.hashCode() : 0);
        hash = 31 * hash + (this.m01 != null ? this.m01.hashCode() : 0);
        hash = 31 * hash + (this.m02 != null ? this.m02.hashCode() : 0);
        hash = 31 * hash + (this.m10 != null ? this.m10.hashCode() : 0);
        hash = 31 * hash + (this.m11 != null ? this.m11.hashCode() : 0);
        hash = 31 * hash + (this.m12 != null ? this.m12.hashCode() : 0);
        hash = 31 * hash + (this.m20 != null ? this.m20.hashCode() : 0);
        hash = 31 * hash + (this.m21 != null ? this.m21.hashCode() : 0);
        hash = 31 * hash + (this.m22 != null ? this.m22.hashCode() : 0);
        return hash;
    }

    @Override
    public String toString() {
        return "M33{" + " [" + m00 + " " + m01 + " " + m02 + "; "
                             + m10 + " " + m11 + " " + m12 + "; "
                             + m20 + " " + m21 + " " + m22 + "] }";
    }

}

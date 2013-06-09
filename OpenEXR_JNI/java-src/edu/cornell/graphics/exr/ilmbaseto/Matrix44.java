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
 * Transfer object for a 4x4 matrix corresponding to
 * {@code Imath::Matrix44<class T>>} in the IlmBase C++ library. The matrix
 * organization is:
 * <pre>
 *     m00 m01 m02 m03
 *     m10 m11 m12 m13
 *     m20 m21 m22 m23
 *     m30 m31 m32 m33
 * </pre>
 */
public class Matrix44<T extends Number> {
    public T m00, m01, m02, m03;
    public T m10, m11, m12, m13;
    public T m20, m21, m22, m23;
    public T m30, m31, m32, m33;
    
    /**
     * Default constructor. Creates an uninitialized matrix.
     */
    public Matrix44() {
        // empty
    }
    
    /**
     * Constructor which sets all the matrix elements to {@code a}.
     * @param a the value for each entry of the new matrix.
     */
    public Matrix44(T a) {
        m00 = m01 = m02 = m03 = a;
        m10 = m11 = m12 = m13 = a;
        m20 = m21 = m22 = m23 = a;
        m30 = m31 = m32 = m33 = a;
    }
    
    /**
     * Copy constructor. Copies each element from {@code other} into the new
     * instance.
     * @param other the other 4x4 matrix.
     */
    public Matrix44(Matrix44<T> other) {
        this.m00 = other.m00;
        this.m01 = other.m01;
        this.m02 = other.m02;
        this.m03 = other.m03;
        
        this.m10 = other.m10;
        this.m11 = other.m11;
        this.m12 = other.m12;
        this.m13 = other.m13;
        
        this.m20 = other.m20;
        this.m21 = other.m21;
        this.m22 = other.m22;
        this.m23 = other.m23;
        
        this.m30 = other.m30;
        this.m31 = other.m31;
        this.m32 = other.m32;
        this.m33 = other.m33;
    }
    
    /**
     * Diagonal matrix constructor. Sets each value in the diagonal
     * to {@code diag} and the off-diagonal values to {@code offDiag}.
     * 
     * @param diag value for the diagonal elements
     * @param offDiag value for the off-diagonal elements
     */
    public Matrix44(T diag, T offDiag) {
        m00 = diag;
        m01 = offDiag;
        m02 = offDiag;
        m03 = offDiag;
        
        m10 = offDiag;
        m11 = diag;
        m12 = offDiag;
        m13 = offDiag;
        
        m20 = offDiag;
        m21 = offDiag;
        m22 = diag;
        m23 = offDiag;
        
        m30 = offDiag;
        m31 = offDiag;
        m32 = offDiag;
        m33 = diag;
    }
    
    /**
     * Explicit per element constructor. Initializes the matrix with
     * specific values; the arguments are in row-major order.
     * 
     * @param m00 value for entry <tt>m<sub>0,0</sub></tt> (row 0, column 0)

     * @param m01 value for entry <tt>m<sub>0,1</sub></tt> (row 0, column 1)
     * @param m02 value for entry <tt>m<sub>0,2</sub></tt> (row 0, column 2)
     * @param m03 value for entry <tt>m<sub>0,3</sub></tt> (row 0, column 3)
     * @param m10 value for entry <tt>m<sub>1,0</sub></tt> (row 1, column 0)
     * @param m11 value for entry <tt>m<sub>1,1</sub></tt> (row 1, column 1)
     * @param m12 value for entry <tt>m<sub>1,2</sub></tt> (row 1, column 2)
     * @param m13 value for entry <tt>m<sub>1,3</sub></tt> (row 1, column 3)
     * @param m20 value for entry <tt>m<sub>2,0</sub></tt> (row 2, column 0)
     * @param m21 value for entry <tt>m<sub>2,1</sub></tt> (row 2, column 1)
     * @param m22 value for entry <tt>m<sub>2,2</sub></tt> (row 2, column 2)
     * @param m23 value for entry <tt>m<sub>2,3</sub></tt> (row 2, column 3)
     * @param m30 value for entry <tt>m<sub>3,0</sub></tt> (row 3, column 0)
     * @param m31 value for entry <tt>m<sub>3,1</sub></tt> (row 3, column 1)
     * @param m32 value for entry <tt>m<sub>3,2</sub></tt> (row 3, column 2)
     * @param m33 value for entry <tt>m<sub>3,3</sub></tt> (row 3, column 3)
     */
    public Matrix44(T m00, T m01, T m02, T m03,
        T m10, T m11, T m12, T m13,
        T m20, T m21, T m22, T m23,
        T m30, T m31, T m32, T m33) {
        this.m00 = m00;
        this.m01 = m01;
        this.m02 = m02;
        this.m03 = m03;
        
        this.m10 = m10;
        this.m11 = m11;
        this.m12 = m12;
        this.m13 = m13;
        
        this.m20 = m20;
        this.m21 = m21;
        this.m22 = m22;
        this.m23 = m23;
        
        this.m30 = m30;
        this.m31 = m31;
        this.m32 = m32;
        this.m33 = m33;
    }

    @Override
    @SuppressWarnings("unchecked")
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Matrix44<T> other = (Matrix44<T>) obj;
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
        if (this.m03 != other.m03 &&
                (this.m03 == null || !this.m03.equals(other.m03))) {
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
        if (this.m13 != other.m13 &&
                (this.m13 == null || !this.m13.equals(other.m13))) {
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
        if (this.m23 != other.m23 &&
                (this.m23 == null || !this.m23.equals(other.m23))) {
            return false;
        }
        if (this.m30 != other.m30 &&
                (this.m30 == null || !this.m30.equals(other.m30))) {
            return false;
        }
        if (this.m31 != other.m31 &&
                (this.m31 == null || !this.m31.equals(other.m31))) {
            return false;
        }
        if (this.m32 != other.m32 &&
                (this.m32 == null || !this.m32.equals(other.m32))) {
            return false;
        }
        if (this.m33 != other.m33 &&
                (this.m33 == null || !this.m33.equals(other.m33))) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 13 * hash + (this.m00 != null ? this.m00.hashCode() : 0);
        hash = 13 * hash + (this.m01 != null ? this.m01.hashCode() : 0);
        hash = 13 * hash + (this.m02 != null ? this.m02.hashCode() : 0);
        hash = 13 * hash + (this.m03 != null ? this.m03.hashCode() : 0);
        hash = 13 * hash + (this.m10 != null ? this.m10.hashCode() : 0);
        hash = 13 * hash + (this.m11 != null ? this.m11.hashCode() : 0);
        hash = 13 * hash + (this.m12 != null ? this.m12.hashCode() : 0);
        hash = 13 * hash + (this.m13 != null ? this.m13.hashCode() : 0);
        hash = 13 * hash + (this.m20 != null ? this.m20.hashCode() : 0);
        hash = 13 * hash + (this.m21 != null ? this.m21.hashCode() : 0);
        hash = 13 * hash + (this.m22 != null ? this.m22.hashCode() : 0);
        hash = 13 * hash + (this.m23 != null ? this.m23.hashCode() : 0);
        hash = 13 * hash + (this.m30 != null ? this.m30.hashCode() : 0);
        hash = 13 * hash + (this.m31 != null ? this.m31.hashCode() : 0);
        hash = 13 * hash + (this.m32 != null ? this.m32.hashCode() : 0);
        hash = 13 * hash + (this.m33 != null ? this.m33.hashCode() : 0);
        return hash;
    }

    @Override
    public String toString() {
        return "M44{" + " [" + m00 + " " + m01 + " " + m02 + " " + m03 + "; "
                             + m10 + " " + m11 + " " + m12 + " " + m13 + "; "
                             + m20 + " " + m21 + " " + m22 + " " + m23 + "; "
                             + m30 + " " + m31 + " " + m32 + " " + m33 + "] }";
    }
    
}

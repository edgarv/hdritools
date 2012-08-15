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
 * Transfer object for specifying a two dimensional range of values. It
 * corresponds to the class {@code Imath::Box<Imath::Vec2<class T>>} of the
 * original IlmBase C++ library.
 */
public class Box2<T extends Number> {
    public T xMin;
    public T yMin;
    public T xMax;
    public T yMax;
    
    public Box2() {}
    
    public Box2(T xMin, T yMin, T xMax, T yMax) {
        this.xMin = xMin;
        this.yMin = yMin;
        this.xMax = xMax;
        this.yMax = yMax;
    }

    @Override
    public String toString() {
        return "(" + xMin + " " + yMin + ") - (" + xMax + " " + yMax + ")";
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Box2<T> other = (Box2<T>) obj;
        if (this.xMin != other.xMin &&
                (this.xMin == null || !this.xMin.equals(other.xMin))) {
            return false;
        }
        if (this.yMin != other.yMin &&
                (this.yMin == null || !this.yMin.equals(other.yMin))) {
            return false;
        }
        if (this.xMax != other.xMax &&
                (this.xMax == null || !this.xMax.equals(other.xMax))) {
            return false;
        }
        if (this.yMax != other.yMax &&
                (this.yMax == null || !this.yMax.equals(other.yMax))) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 5;
        hash = 17 * hash + (this.xMin != null ? this.xMin.hashCode() : 0);
        hash = 17 * hash + (this.yMin != null ? this.yMin.hashCode() : 0);
        hash = 17 * hash + (this.xMax != null ? this.xMax.hashCode() : 0);
        hash = 17 * hash + (this.yMax != null ? this.yMax.hashCode() : 0);
        return hash;
    }
    
}

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

// TODO: Add documentation
public class Channel {
    
    public static enum PixelType {
        UINT,
        HALF,
        FLOAT
    }
    
    public String name;
    public PixelType type;
    public boolean pLinear;
    public int xSampling;
    public int ySampling;

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Channel other = (Channel) obj;
        if ((this.name == null) ?
                (other.name != null) : !this.name.equals(other.name)) {
            return false;
        }
        if (this.type != other.type) {
            return false;
        }
        if (this.pLinear != other.pLinear) {
            return false;
        }
        if (this.xSampling != other.xSampling) {
            return false;
        }
        if (this.ySampling != other.ySampling) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 83 * hash + (this.name != null ? this.name.hashCode() : 0);
        hash = 83 * hash + (this.type != null ? this.type.hashCode() : 0);
        hash = 83 * hash + (this.pLinear ? 1 : 0);
        hash = 83 * hash + this.xSampling;
        hash = 83 * hash + this.ySampling;
        return hash;
    }

}

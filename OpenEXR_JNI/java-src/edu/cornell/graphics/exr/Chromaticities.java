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
public class Chromaticities {
    public float redX,   redY;
    public float greenX, greenY;
    public float blueX,  blueY;
    public float whiteX, whiteY;
    
    public Chromaticities() {}

    public Chromaticities(float redX, float redY, float greenX, float greenY,
            float blueX, float blueY, float whiteX, float whiteY) {
        this.redX   = redX;
        this.redY   = redY;
        this.greenX = greenX;
        this.greenY = greenY;
        this.blueX  = blueX;
        this.blueY  = blueY;
        this.whiteX = whiteX;
        this.whiteY = whiteY;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Chromaticities c = (Chromaticities) obj;
        if (Float.floatToIntBits(redX) != Float.floatToIntBits(c.redX)) {
            return false;
        }
        if (Float.floatToIntBits(redY) != Float.floatToIntBits(c.redY)) {
            return false;
        }
        if (Float.floatToIntBits(greenX) != Float.floatToIntBits(c.greenX)) {
            return false;
        }
        if (Float.floatToIntBits(greenY) != Float.floatToIntBits(c.greenY)) {
            return false;
        }
        if (Float.floatToIntBits(blueX) != Float.floatToIntBits(c.blueX)) {
            return false;
        }
        if (Float.floatToIntBits(blueY) != Float.floatToIntBits(c.blueY)) {
            return false;
        }
        if (Float.floatToIntBits(whiteX) != Float.floatToIntBits(c.whiteX)) {
            return false;
        }
        if (Float.floatToIntBits(whiteY) != Float.floatToIntBits(c.whiteY)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 23 * hash + Float.floatToIntBits(this.redX);
        hash = 23 * hash + Float.floatToIntBits(this.redY);
        hash = 23 * hash + Float.floatToIntBits(this.greenX);
        hash = 23 * hash + Float.floatToIntBits(this.greenY);
        hash = 23 * hash + Float.floatToIntBits(this.blueX);
        hash = 23 * hash + Float.floatToIntBits(this.blueY);
        hash = 23 * hash + Float.floatToIntBits(this.whiteX);
        hash = 23 * hash + Float.floatToIntBits(this.whiteY);
        return hash;
    }
    
}

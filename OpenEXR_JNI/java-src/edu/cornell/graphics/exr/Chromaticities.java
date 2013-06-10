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
 * The CIE {@code x} and {@code y} coordinates of the RGB triplets
 * (1,0,0), (0,1,0), (0,0,1) and (1,1,1).
 * 
 * @since OpenEXR-JNI 2.1
 */
public class Chromaticities {
    /** CIE {@code x} coordinate of the RGB triplet (1,0,0). */
    public float redX;
    /** CIE {@code y} coordinate of the RGB triplet (1,0,0). */
    public float redY;
    /** CIE {@code x} coordinate of the RGB triplet (0,1,0). */
    public float greenX;
    /** CIE {@code y} coordinate of the RGB triplet (0,1,0). */
    public float greenY;
    /** CIE {@code x} coordinate of the RGB triplet (0,0,1). */
    public float blueX;
    /** CIE {@code y} coordinate of the RGB triplet (0,0,1). */
    public float blueY;
    /** CIE {@code x} coordinate of the RGB triplet (1,1,1). */
    public float whiteX;
    /** CIE {@code y} coordinate of the RGB triplet (1,1,1). */
    public float whiteY;
    
    /**
     * Construct a new {@code Chromaticities} instance according to the
     * Rec. ITU-R BT.709-3 primaries.
     */
    public Chromaticities() {
        this.redX   = 0.6400f;
        this.redY   = 0.3300f;
        this.greenX = 0.3000f;
        this.greenY = 0.6000f;
        this.blueX  = 0.1500f;
        this.blueY  = 0.0600f;
        this.whiteX = 0.3127f;
        this.whiteY = 0.3290f;
    }
    
    /**
     * Constructs a new {@code Chromaticities} instance with the same
     * CIE coordinates as {@code other}.
     * 
     * @param other a non-null {@code Chromaticities} object
     */
    public Chromaticities(Chromaticities other) {
        this.redX   = other.redX;
        this.redY   = other.redY;
        this.greenX = other.greenX;
        this.greenY = other.greenY;
        this.blueX  = other.blueX;
        this.blueY  = other.blueY;
        this.whiteX = other.whiteX;
        this.whiteY = other.whiteY;
    }

    /**
     * Constructs a new {@code Chromaticities}instance with specific
     * CIE coordinates.
     * 
     * @param redX   CIE {@code x} coordinate of the RGB triplet (1,0,0)
     * @param redY   CIE {@code y} coordinate of the RGB triplet (1,0,0)
     * @param greenX CIE {@code x} coordinate of the RGB triplet (0,1,0)
     * @param greenY CIE {@code y} coordinate of the RGB triplet (0,1,0)
     * @param blueX  CIE {@code x} coordinate of the RGB triplet (0,0,1)
     * @param blueY  CIE {@code y} coordinate of the RGB triplet (0,0,1)
     * @param whiteX CIE {@code x} coordinate of the RGB triplet (1,1,1)
     * @param whiteY CIE {@code y} coordinate of the RGB triplet (1,1,1)
     */
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

    /**
     * Compares this chromaticities instance to the specified object. The result
     * is {@code true} if and only if the argument is not {@code null}, is a
     * {@code Chromaticities} object and all their CIE coordinates are the same.
     *
     * @param obj The object to compare this instance against
     * @return {@code true} if the given object represents a
     *         {@code Chromaticities} equivalent to this instance,
     *         {@code false} otherwise
     */
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

    /**
     * Returns a hash code for this chromaticities instance. The hash code
     * is computes by an aggregate of each of its CIE coordinates.
     * 
     * @return a hash code for this chromaticities instance
     */
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

    /**
     * Returns a string representation of the chromaticities.
     * @return a string representation of the chromaticities
     */
    @Override
    public String toString() {
        return "Chromaticities{" +
                "red=("      + redX   + ',' + redY +
                ") green=(" + greenX + ',' + greenY + 
                ") blue=("  + blueX  + ',' + blueY + 
                ") white=(" + whiteX + ',' + whiteY + ")}";
    }
    
}

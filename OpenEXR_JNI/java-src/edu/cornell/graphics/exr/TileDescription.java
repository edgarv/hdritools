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
public class TileDescription implements Cloneable {
    
    public static enum LevelMode {
        ONE_LEVEL,
        MIPMAP_LEVELS,
        RIPMAP_LEVELS
    }
    
    public static enum RoundingMode {
        ROUND_DOWN,
        ROUND_UP
    }
    
    public int xSize;
    public int ySize;
    public LevelMode mode;
    public RoundingMode roundingMode;

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final TileDescription other = (TileDescription) obj;
        if (this.xSize != other.xSize) {
            return false;
        }
        if (this.ySize != other.ySize) {
            return false;
        }
        if (this.mode != other.mode) {
            return false;
        }
        if (this.roundingMode != other.roundingMode) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 3;
        hash = 29 * hash + this.xSize;
        hash = 29 * hash + this.ySize;
        hash = 29 * hash + (this.mode != null ? this.mode.hashCode() : 0);
        hash = 29 * hash + (this.roundingMode != null ?
                this.roundingMode.hashCode() : 0);
        return hash;
    }

    @Override
    public TileDescription clone() {
        try {
            return (TileDescription) super.clone();
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

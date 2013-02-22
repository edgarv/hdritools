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

import java.util.Arrays;

// TODO: Add documentation
public class PreviewImage implements Cloneable {
    public int width;
    public int height;
    public byte[] pixelData;

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 67 * hash + this.width;
        hash = 67 * hash + this.height;
        hash = 67 * hash + Arrays.hashCode(this.pixelData);
        return hash;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final PreviewImage other = (PreviewImage) obj;
        if (this.width != other.width) {
            return false;
        }
        if (this.height != other.height) {
            return false;
        }
        if (!Arrays.equals(this.pixelData, other.pixelData)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "PreviewImage{" + "width=" + width + ", height=" + height + '}';
    }

    @Override
    public PreviewImage clone() {
        try {
            PreviewImage p = (PreviewImage) super.clone();
            p.pixelData = this.pixelData.clone();
            return p;
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

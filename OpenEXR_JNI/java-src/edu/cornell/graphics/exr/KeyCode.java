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
public class KeyCode {
    public int filmMfcCode;
    public int filmType;
    public int prefix;
    public int count;
    public int perfOffset;
    public int perfsPerFrame;
    public int perfsPerCount;
    
    public KeyCode() {}

    public KeyCode(int filmMfcCode, int filmType, int prefix, int count,
            int perfOffset, int perfsPerFrame, int perfsPerCount) {
        this.filmMfcCode   = filmMfcCode;
        this.filmType      = filmType;
        this.prefix        = prefix;
        this.count         = count;
        this.perfOffset    = perfOffset;
        this.perfsPerFrame = perfsPerFrame;
        this.perfsPerCount = perfsPerCount;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final KeyCode other = (KeyCode) obj;
        if (this.filmMfcCode != other.filmMfcCode) {
            return false;
        }
        if (this.filmType != other.filmType) {
            return false;
        }
        if (this.prefix != other.prefix) {
            return false;
        }
        if (this.count != other.count) {
            return false;
        }
        if (this.perfOffset != other.perfOffset) {
            return false;
        }
        if (this.perfsPerFrame != other.perfsPerFrame) {
            return false;
        }
        if (this.perfsPerCount != other.perfsPerCount) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 61 * hash + this.filmMfcCode;
        hash = 61 * hash + this.filmType;
        hash = 61 * hash + this.prefix;
        hash = 61 * hash + this.count;
        hash = 61 * hash + this.perfOffset;
        hash = 61 * hash + this.perfsPerFrame;
        hash = 61 * hash + this.perfsPerCount;
        return hash;
    }
    
}

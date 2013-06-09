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

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

package edu.cornell.graphics.exr;

/**
 * Description of the data contained in a single given channel of an OpenEXR
 * file, including its storage data type, sampling factors and perception hints.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class Channel {
    
    /**
     * Pixel data type.
     */
    public PixelType type = PixelType.HALF;
    
    /**
     * Hint to lossy compression methods that indicates whether human perception
     * of the quantity represented by this channel is closer to linear or closer
     * to logarithmic. Compression methods may optimize image quality by
     * adjusting pixel data quantization according to this hint.
     * 
     * <p>For example, perception of rec, green blue and luminance is
     * approximately logarithmic; the difference between 0.1 and 0.2 is
     * perceived to be roughly the same as the difference between 1.0 and 2.0.
     * Perception of chroma coordinates tends to be closer to linear than
     * logarithmic; the difference between 0.1 and 0.2 is perceived to be
     * roughly the same as the difference between 1.0 and 1.1.
     */
    public boolean pLinear = false;
    
    /**
     * Subsampling: pixel {@code (x,y)} is present in the channel only if
     * {@code (x % xSampling == 0 && y % ySampling == 0)}.
     * @see #ySampling
     */
    public int xSampling = 1;
    
    /**
     * Subsampling: pixel {@code (x,y)} is present in the channel only if
     * {@code (x % xSampling == 0 && y % ySampling == 0)}.
     * @see #xSampling
     */
    public int ySampling = 1;
    
    /** Default constructor */
    public Channel() {
        // empty
    }
    
    /**
     * Constructor with an specific pixel type. Both sampling factors remain at
     * their default value of {@literal 1} and {@code pLinear} is set to
     * {@code false}.
     * 
     * @param type the non-null pixel type for the channel
     */
    public Channel(PixelType type) {
        if (type == null) {
            throw new IllegalArgumentException("null pixel type");
        }
        this.type = type;
    }
    
    /** Copy constructor */
    public Channel(Channel other) {
        if (other == null) {
            throw new IllegalArgumentException("null source channel");
        }
        type      = other.type;
        pLinear   = other.pLinear;
        xSampling = other.xSampling;
        ySampling = other.ySampling;
    }
    
    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Channel other = (Channel) obj;
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
        hash = 59 * hash + (this.type != null ? this.type.hashCode() : 0);
        hash = 59 * hash + (this.pLinear ? 1 : 0);
        hash = 59 * hash + this.xSampling;
        hash = 59 * hash + this.ySampling;
        return hash;
    }

    @Override
    public String toString() {
        if (xSampling != 1 || ySampling != 1 || pLinear) {
            return "Channel{" + type + " (" + xSampling + ',' + ySampling + ")"
                    + (pLinear ? " pLinear" : "") + '}';
        } else {
            return "Channel{" + type + '}';
        }
    }

}

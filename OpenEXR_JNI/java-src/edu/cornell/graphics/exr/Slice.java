/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2013 Program of Computer Graphics, Cornell University

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

import java.nio.ByteBuffer;

/**
 * Description of a single slice of the frame buffer.
 * 
 * <p>This is simply a transfer object, it does <em>NOT</em> check any field
 * for correctness or even coherence. Other classes which use slices will be
 * responsible of verifying that a slice is valid.</p>
 * 
 * <p><b>Memory layout:</b> the address of pixel {@code (x,y)}, relative to the
 * current {@code buffer.position()} is:
 * <pre>    baseOffset + (xp / xSampling) * xStride + (yp / ySampling) * yStride</pre>
 * Where {@code xp} and {@code yp} are computed as follows:
 * <ul>
 *   <li>If we are reading or writing a scanline-based file:
 *   <pre>
 * xp = x
 * yp = y</pre>
 *   </li>
 *   <li>If we are reading a tile whose upper left corner is at {@code (xt,yt)}:
 *   <pre>
 * if xTileCoords is true then xp = x - xt, else xp = x
 * if yTileCoords is true then yp = y - yt, else yp = y</pre>
 *   </li>
 * </ul>
 * Setting {@code baseOffset} appropriately according to the file layout is more
 * important than it seems at first sight. Pixels in OpenEXR files may have
 * negative coordinates, thus {@code baseOffset} must compensate for that so
 * that the memory location of a pixel maps to a valid position within the
 * buffer. Other use of this parameter is when reading/writing only a fragment
 * of a frame buffer: it allows to map only a subset of the image data into
 * an appropriate region of the buffer.</p>
 * 
 * <p><b>Subsampling:</b> pixel {@code (x,y)} is present in the slice only if
 * <pre>    x % xSampling == 0 && y % ySampling == 0</pre></p>
 * 
 * <p>Note on terminology: as a part of a file, a component of an image (e.g.
 * red, green, blue, depth, etc.) is called a <em>channel</em>. As part of a
 * frame buffer is called a <em>slice</em>.
 */
public class Slice {
  
    /**
     * Helper class to provide a fluent-style interface for creating slices.
     */
    public final static class SliceBuilder {

        private final Slice slice = new Slice();

        private SliceBuilder() {
        }

        public SliceBuilder pixelType(PixelType type) {
            slice.type = type;
            return this;
        }

        public SliceBuilder buffer(ByteBuffer buf) {
            slice.buffer = buf;
            return this;
        }

        public SliceBuilder baseOffset(int off) {
            slice.baseOffset = off;
            return this;
        }

        public SliceBuilder xStride(int numBytes) {
            slice.xStride = numBytes;
            return this;
        }

        public SliceBuilder yStride(int numBytes) {
            slice.yStride = numBytes;
            return this;
        }

        public SliceBuilder xSampling(int samplingInterval) {
            slice.xSampling = samplingInterval;
            return this;
        }

        public SliceBuilder ySampling(int samplingInterval) {
            slice.ySampling = samplingInterval;
            return this;
        }

        public SliceBuilder fillValue(double value) {
            slice.fillValue = value;
            return this;
        }
        
        public SliceBuilder xTileCoords(boolean value) {
            slice.xTileCoords = value;
            return this;
        }
        
        public SliceBuilder yTileCoords(boolean value) {
            slice.yTileCoords = value;
            return this;
        }

        public Slice get() {
            return slice;
        }
    }

    
    /** Private default constructor to motivate using the slice builder */
    private Slice() {
        // empty
    }

    /** Copy constructor */
    public Slice(Slice other) {
        type        = other.type;
        buffer      = other.buffer;
        baseOffset  = other.baseOffset;
        xStride     = other.xStride;
        yStride     = other.yStride;
        xSampling   = other.xSampling;
        ySampling   = other.ySampling;
        fillValue   = other.fillValue;
        xTileCoords = other.xTileCoords;
        yTileCoords = other.yTileCoords;
    }
    
    /** Data type */
    public PixelType type = PixelType.HALF;
    
    /** Buffer containing the actual slice data */
    ByteBuffer buffer = null;
    
    /**
     * Offset within the buffer, used when calculating the address of a pixel.
     */
    int baseOffset = 0;
    
    /**
     * Distance in bytes between pixels {@code (x,y)} and {@code (x+1,y)}. It
     * may be a negative number.
     */
    int xStride = 0;
    
    /**
     * Distance in bytes between pixels {@code (x,y)} and {@code (x,y+1)}. It
     * may be a negative number.
     */
    int yStride = 0;
    
    /** Pixel sampling interval along {@code x} (the image width.) */
    int xSampling = 1;
    
    /** Pixel sampling interval along {@code y} (the image height.) */
    int ySampling = 1;
    
    /**
     * Default value, used to fill the slice when a file without a channel that
     * corresponds to this slice is read.
     */
    double fillValue = 0.0;
    
    /**
     * {@code x} absolute/relative addressing flag for tiled files.
     * 
     * <p>For tiled files, if {@code true} pixel addressing is performed using
     * absolute coordinates for {@code x}. If {@code false}, {@code x}
     * coordinates are relative to the tile's upper left corner.
     * For scanline-based files this flag has no effect; pixel addressing is
     * always done using absolute coordinates.</p>
     */
    boolean xTileCoords = false;
    
    /**
     * {@code y} absolute/relative addressing flag for tiled files.
     * 
     * <p>For tiled files, if {@code true} pixel addressing is performed using
     * absolute coordinates for {@code y}. If {@code false}, {@code y}
     * coordinates are relative to the tile's upper left corner.
     * For scanline-based files this flag has no effect; pixel addressing is
     * always done using absolute coordinates.</p>
     */
    boolean yTileCoords = false;
    
    /**
     * Syntactic-sugar method which return a {@code SliceBuilder} to 
     * configure a new {@code Slice} instance.
     * 
     * @return a {@code SliceBuilder} for configuring a new {@code Slice}.
     */
    public static SliceBuilder build() {
        return new SliceBuilder();
    }

    /**
     * Returns the current hash code of this slice.
     * 
     * <p>The hash code of a slice uses all of its components in the standard
     * way except for the buffer: for two slices to have the same hash code
     * they must reference the same buffer object, and the buffer's position and
     * limit must remain unchanged between calls to {@code hashCode}. The
     * actual contents of the buffer are ignored.</p>
     * 
     * <p>Because of the weak conditions for equality, slices should not be used
     * as keys in hash maps or similar structures.</p>
     * 
     * @return the current hash code of this slice
     */
    @Override
    public int hashCode() {
        int hash = 7;
        hash = 67 * hash + (this.type != null ? this.type.hashCode() : 0);
        if (this.buffer != null) {
            hash = 67 * hash + System.identityHashCode(this.buffer);
            hash = 67 * hash + this.buffer.position();
            hash = 67 * hash + this.buffer.limit();
        }
        hash = 67 * hash + this.baseOffset;
        hash = 67 * hash + this.xStride;
        hash = 67 * hash + this.yStride;
        hash = 67 * hash + this.xSampling;
        hash = 67 * hash + this.ySampling;
        hash = 67 * hash + (int) (Double.doubleToLongBits(this.fillValue) ^ 
                (Double.doubleToLongBits(this.fillValue) >>> 32));
        hash = 67 * hash + (this.xTileCoords ? 1 : 0);
        hash = 67 * hash + (this.yTileCoords ? 1 : 0);
        return hash;
    }

    /**
     * Tells whether or not this slice is equal to another object.
     * 
     * <p>Two slices are equal if, and only if,
     * <ol>
     *   <li>They have the same baseOffset, pixel type, x/y stride, x/y sampling 
     *   factors, fill value and x/y tile addressing mode.</li>
     *   <li>They reference exactly the same buffer object, hence the weak
     *   equivalence expression {@code this.buffer == ((Slice)obj).buffer} will
     *   be {@code true}.</li>
     * </ol>
     * A slice is not equal to any other type of object.</p>
     * <p>Note that even if two slices are equal, their hash codes may change
     * depending on their buffer's current position and limit.</p>
     * 
     * @param obj the object to which this slice is to be compared
     * @return {@code true} if, and only if, this slice is equal to the
     *         given object
     * @see #hashCode() 
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final Slice other = (Slice) obj;
        if (this.type != other.type) {
            return false;
        }
        if (this.buffer != other.buffer) {
            return false;
        }
        if (this.baseOffset != other.baseOffset) {
            return false;
        }
        if (this.xStride != other.xStride) {
            return false;
        }
        if (this.yStride != other.yStride) {
            return false;
        }
        if (this.xSampling != other.xSampling) {
            return false;
        }
        if (this.ySampling != other.ySampling) {
            return false;
        }
        if (Double.doubleToLongBits(this.fillValue) !=
            Double.doubleToLongBits(other.fillValue)) {
            return false;
        }
        if (this.xTileCoords != other.xTileCoords) {
            return false;
        }
        if (this.yTileCoords != other.yTileCoords) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "Slice{" + "type=" + type + ", base=" + baseOffset +
                ", stride=(" + xStride + ',' + yStride + ')' +
                ((xSampling == 1 && ySampling == 1) ? "" :
                (", sampling=(" + xSampling + ',' + ySampling + ')')) +
                ", fill=" + fillValue +
                ((!xTileCoords && !yTileCoords) ? "" :
                (", tileCoords=(" + xTileCoords + ',' + yTileCoords + ')')) +
                '}';
    }
    
}

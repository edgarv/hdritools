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
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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
 * Magic and version number constants
 */
public class EXRVersion {
    
    /**
     * The MAGIC number is stored in the first four bytes of every OpenEXR image
     * file. This can be used to quickly test whether a given file is an OpenEXR
     * image file (see {@link #isImfMagic(byte[], int) }).
     */
    public final static int MAGIC = 20000630;
    
    //
    // The second item in each OpenEXR image file, right after the
    // magic number, is a four-byte file version identifier. Depending
    // on a file's version identifier, a file reader can enable various
    // backwards-compatibility switches, or it can quickly reject files
    // that it cannot read.
    //
    // The version identifier is split into an 8-bit version number,
    // and a 24-bit flags field.
    //
    
    /**
     * Mask for the 8-bit version number inside the four-byte version identifier
     * which follows the magic number.
     */
    public final static int VERSION_NUMBER_FIELD = 0x000000ff;
    
    /**
     * Mask for the 24-bit flags field inside the four-byte version identifier
     * which follows the magic number.
     */
    public final static int VERSION_FLAGS_FIELD  = 0xffffff00;
    
    /** Value that goes into {@link #VERSION_NUMBER_FIELD} */
    public final static int EXR_VERSION = 2;
    
    //
    // Flags that can go into VERSION_FLAGS_FIELD.
    // Flags can only occupy the 1 bits in VERSION_FLAGS_FIELD.
    //
    
    /** File is tiled */
    public final static int TILED_FLAG           = 0x00000200;
    
    /** File contains long attribute or channel names */
    public final static int LONG_NAMES_FLAG      = 0x00000400;
    
    /**
     * File has at least one part which is not a regular scanline image or
     * regular tiled image (that is, it is a deep format).
     */
    public final static int NON_IMAGE_FLAG       = 0x00000800;
    
    /** File has multiple parts */
    public final static int MULTI_PART_FILE_FLAG = 0x00001000;
    
    /** Bitwise OR of all supported flags */
    public final static int SUPPORTED_FLAGS = TILED_FLAG | LONG_NAMES_FLAG;
    
    /** Bitwise OR of all known flags */
    public final static int ALL_FLAGS = TILED_FLAG | LONG_NAMES_FLAG |
            NON_IMAGE_FLAG | MULTI_PART_FILE_FLAG;
    
    //
    // Utility functions
    //
    
    public static int getMaxNameLength(int version) {
        return (version & LONG_NAMES_FLAG) != 0 ? 255 : 31;
    }
    
    public static boolean isTiled(int version) {
        return (version & TILED_FLAG) != 0;
    }
    
    public static boolean isMultiPart(int version) {
        return (version & MULTI_PART_FILE_FLAG) != 0;
    }
    
    public static boolean isNonImage(int version) {
        return (version & NON_IMAGE_FLAG) != 0;
    }
    
    public static int makeTiled(int version) {
        return version | TILED_FLAG;
    }
    
    public static int makeNotTiled(int version) {
        return version & ~TILED_FLAG;
    }
    
    public static int getVersion(int version) {
        return version & VERSION_NUMBER_FIELD;
    }
    
    public static int getFlags(int version) {
        return version & VERSION_FLAGS_FIELD;
    }
    
    public static boolean supportsFlags(int flags) {
        return (flags & ~SUPPORTED_FLAGS) == 0;
    }
    
    
    
    public static boolean isImfMagic(byte[] bytes) {
        return isImfMagic(bytes, 0);
    }
    
    public static boolean isImfMagic(byte[] bytes, int offset) {
        if (offset < 0) {
            throw new IllegalArgumentException("Invalid offset: " + offset);
        } else if (bytes.length - offset < 4) {
            throw new IllegalArgumentException("Not enough data.");
        }
        return bytes[offset]   == ( MAGIC        & 0x00ff) &&
               bytes[offset+1] == ((MAGIC >>  8) & 0x00ff) &&
               bytes[offset+2] == ((MAGIC >> 16) & 0x00ff) &&
               bytes[offset+3] == ((MAGIC >> 24) & 0x00ff);
    }
}

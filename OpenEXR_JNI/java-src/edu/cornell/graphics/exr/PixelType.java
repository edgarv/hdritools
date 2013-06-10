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

/**
 * Defines the possible data types for an image channel.
 * 
 * @since OpenEXR-JNI 3.0
 */
public enum PixelType {
    /** 
     * 32-bit unsigned integers. For discrete per-pixel data such as 
     * object identifiers.
     */
    UINT(4),
    
    /** 
     * 16-bit IEEE 754-2008 floating-point numbers. For regular image data.
     * @see Half
     */
    HALF(2),
    
    /** 
     * 32-bit IEEE 754 floating-point numbers. Used where the range or 
     * precision of 16-bit number is not sufficient, for example,
     * depth channels.
     */
    FLOAT(4);
    
    
    private final int bytes;
    
    PixelType(int bytes) {
        this.bytes = bytes;
    }
    
    /**
     * Returns the number of bytes used to represent a single value of this type.
     * @return the number of bytes used to represent a single value of this type
     */
    public int byteSize() {
        return bytes;
    }
    
    /**
     * Returns the number of bits used to represent a single value of this type.
     * @return the number of bits used to represent a single value of this type
     */
    public int size() {
        return bytes * 8;
    }
    
}

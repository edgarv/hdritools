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
 * A {@code KeyCode} object uniquely identifies a motion picture film frame.
 * 
 * <p>The following fields specify film manufacturer, film type, film
 * roll and the frame's position within the roll:
 * <table border="1">
 * <tr>
 *   <th>Field</th>
 *   <th>Description</th>
 * </tr>
 * <tr>
 *   <td>{@code filmMfcCode}</td>
 *   <td>film manufacturer code<br/>range: 0 - 99</td>
 * </tr>
 * <tr>
 *   <td>{@code filmType}</td>
 *   <td>film type code<br/>range: 0 - 99</td>
 * </tr>
 * <tr>
 *   <td>{@code prefix}</td>
 *   <td>prefix to identify film roll<br/>range: 0 - 999999</td>
 * </tr>
 * <tr>
 *   <td>{@code count}</td>
 *   <td>count, increments once every {@code perfsPerCount} perforations<br/>
 *    range: 0 - 9999</td>
 * </tr>
 * <tr>
 *   <td>{@code perfOffset}</td>
 *   <td>offset of frame, in perforations from zero-frame reference mark<br/>
 *    range: 0 - 119</td>
 * </tr>
 * <tr>
 *   <td>{@code perfsPerFrame}</td>
 *   <td><p>number of perforations per frame<br/>range: 1 - 15</p>
 *   <p>Typical values:
 *   <ul>
 *     <li>1 for 16mm film</li>
 *     <li>3, 4, or 8 for 35mm film</li>
 *     <li>5, 8 or 15 for 65mm film</li>
 *   </ul></p></td>
 * </tr>
 * <tr>
 *   <td>{@code perfsPerCoun}</td>
 *   <td><p>number of perforations per count<br/>range: 20 - 120</p>
 *   <p>Typical values:
 *   <ul>
 *     <li>20 for 16mm film</li>
 *     <li>64 for 35mm film</li>
 *     <li>80 or 120 for 65mm film</li>
 *   </ul></p></td>
 * </tr>
 * </table></p>
 * 
 * <p>For more information about the interpretation of those fields see
 * the following standards and recommended practice publications:
 * 
 * <blockquote><table border="0">
 * <tr>
 *   <td>SMPTE 254</td>
 *   <td>Motion-Picture Film (35-mm) - Manufacturer-Printed
 *   Latent Image Identification Information</td>
 * </tr><tr>
 *   <td>SMPTE 268M</td>
 *   <td>File Format for Digital Moving-Picture Exchange (DPX)
 *   (section 6.1)</td>
 * </tr><tr>
 *   <td>SMPTE 270</td>
 *   <td>Motion-Picture Film (65-mm) - Manufacturer- Printed
 *   Latent Image Identification Information</td> 
 * </tr><tr>
 *   <td>SMPTE 271</td>
 *   <td>Motion-Picture Film (16-mm) - Manufacturer- Printed
 *   Latent Image Identification Information</td>
 * </tr></table></blockquote></p>
 * 
 * @since 2.2
 */
public class KeyCode {
    /** Film manufacturer code */
    public int filmMfcCode;
    /** Film type code */
    public int filmType;
    /** Prefix to identify film roll */
    public int prefix;
    /** Count, increments once every {@code perfsPerCount} perforations */
    public int count;
    /** offset of frame, in perforations from zero-frame reference mark */
    public int perfOffset;
    /** number of perforations per frame */
    public int perfsPerFrame;
    /** number of perforations per count */
    public int perfsPerCount;
    
    /**
     * Constructs a new {@code KeyCode}. Initializes {@code perfsPerFrame} to 4 
     * and {@code perfsPerCount} to 64, values suitable for 35mm film. All other
     * fields are set to zero.
     */
    public KeyCode() {
        perfsPerFrame = 4;
        perfsPerCount = 64;
    }
    
    /**
     * Copy constructor. Initializes each field of the newly created
     * {@code KeyCode} with those from {@code other}.
     * 
     * @param other a non-null {@code KeyCode}
     */
    public KeyCode(KeyCode other) {
        this.filmMfcCode   = other.filmMfcCode;
        this.filmType      = other.filmType;
        this.prefix        = other.prefix;
        this.count         = other.count;
        this.perfOffset    = other.perfOffset;
        this.perfsPerFrame = other.perfsPerFrame;
        this.perfsPerCount = other.perfsPerCount;
    }

    /**
     * Constructs a new {@code KeyCode} using specific initial values for
     * each field.
     * 
     * @param filmMfcCode film manufacturer code
     * @param filmType film type code
     * @param prefix prefix to identify film roll
     * @param count count, increments once every
     *        {@code perfsPerCount} perforations
     * @param perfOffset offset of frame, in perforations from zero-frame
     *        reference mark
     * @param perfsPerFrame number of perforations per frame
     * @param perfsPerCount number of perforations per count
     */
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

    /**
     * Returns a hash code value for the {@code KeyCode}. The hash code is
     * computed using the values of all fields.
     * 
     * @return a hash code value for the {@code KeyCode}
     */
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

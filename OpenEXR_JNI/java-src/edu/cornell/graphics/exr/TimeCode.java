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
 * A {@code TimeCode} object stores time and control codes as described in
 * SMPTE standard 12M-1999.
 * 
 * <p>A {@code TimeCode} object contains the following fields:
 * 
 * <table border="1" summary="TimeCode fields">
 * <tr>
 *   <th colspan="2">Time address</th>
 * </tr>
 * <tr>
 *   <td>hours</td>
 *   <td>integer, range 0 - 23</td>
 * </tr>
 * <tr>
 *   <td>minutes</td>
 *   <td>integer, range 0 - 59</td>
 * </tr>
 * <tr>
 *   <td>seconds</td>
 *   <td>integer, range 0 - 59</td>
 * </tr>
 * <tr>
 *   <td>frame</td>
 *   <td>integer, range 0 - 29</td>
 * </tr>
 * 
 * <tr>
 *   <th colspan="2">Flags</th>
 * </tr>
 * <tr>
 *   <td>drop frame flag</td>
 *   <td>boolean</td>
 * </tr>
 * <tr>
 *   <td>color frame flag</td>
 *   <td>boolean</td>
 * </tr>
 * <tr>
 *   <td>field/phase frame flag</td>
 *   <td>boolean</td>
 * </tr>
 * <tr>
 *   <td>bgf0</td>
 *   <td>boolean</td>
 * </tr>
 * <tr>
 *   <td>bgf1</td>
 *   <td>boolean</td>
 * </tr>
 * <tr>
 *   <td>bgf2</td>
 *   <td>boolean</td>
 * </tr>
 * 
 * <tr>
 *   <th colspan="2">Binary groups for user-defined data and control codes</th>
 * </tr>
 * <tr>
 *   <td>binary group 1</td>
 *   <td>integer, range 0 - 15</td>
 * </tr>
 * <tr>
 *   <td>binary group 2</td>
 *   <td>integer, range 0 - 15</td>
 * </tr>
 * <tr>
 *   <td colspan="2">&hellip;</td>
 * </tr>
 * <tr>
 *   <td>binary group 8</td>
 *   <td>integer, range 0 - 15</td>
 * </tr>
 * </table>
 * 
 * <p>Class {@code TimeCode} contains methods to convert between the fields
 * listed above and a more compact representation where the fields are packed
 * into two 32-bit integers. In the packed integer representations, bit 0 is the
 * least significant bit, and bit 31 is the most significant bit of the integer
 * value.</p>
 * 
 * <p>The time address and flags fields can be packed in three different ways:
 * <table border="1" summary="TimeCode fileds bit packing">
 * <tr>
 *   <th>bits</th>
 *   <th>packing for 24-frame film</th>
 *   <th>packing for 60-field television</th>
 *   <th>packing for 50-field television</th>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;0 - &nbsp;3</code></td>
 *   <td>frame units</td>
 *   <td>frame units</td>
 *   <td>frame units</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;4 - &nbsp;5</code></td>
 *   <td>frame tens</td>
 *   <td>frame tens</td>
 *   <td>frame tens</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;6</code></td>
 *   <td>unused, set to 0</td>
 *   <td>drop frame flag</td>
 *   <td>unused, set to 0</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;7</code></td>
 *   <td>unused, set to 0</td>
 *   <td>color frame flag</td>
 *   <td>color frame flag</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;8 - 11</code></td>
 *   <td>seconds units</td>
 *   <td>seconds units</td>
 *   <td>seconds units</td>
 * </tr>
 * <tr>
 *   <td>{@code 12 - 14}</td>
 *   <td>seconds tens</td>
 *   <td>seconds tens</td>
 *   <td>seconds tens</td>
 * </tr>
 * <tr>
 *   <td>{@code 15}</td>
 *   <td>phase flag</td>
 *   <td>field/phase flag</td>
 *   <td>bgf0</td>
 * </tr>
 * <tr>
 *   <td>{@code 16 - 19}</td>
 *   <td>minutes units</td>
 *   <td>minutes units</td>
 *   <td>minutes units</td>
 * </tr>
 * <tr>
 *   <td>{@code 20 - 22}</td>
 *   <td>minutes tens</td>
 *   <td>minutes tens</td>
 *   <td>minutes tens</td>
 * </tr>
 * <tr>
 *   <td>{@code 23}</td>
 *   <td>bgf0</td>
 *   <td>bgf0</td>
 *   <td>bgf2</td>
 * </tr>
 * <tr>
 *   <td>{@code 24 - 27}</td>
 *   <td>hours units</td>
 *   <td>hours units</td>
 *   <td>hours units</td>
 * </tr>
 * <tr>
 *   <td>{@code 28 - 29}</td>
 *   <td>hours tens</td>
 *   <td>hours tens</td>
 *   <td>hours tens</td>
 * </tr>
 * <tr>
 *   <td>{@code 30}</td>
 *   <td>bgf1</td>
 *   <td>bgf1</td>
 *   <td>bgf1</td>
 * </tr>
 * <tr>
 *   <td>{@code 31}</td>
 *   <td>bgf2</td>
 *   <td>bgf2</td>
 *   <td>field/phase flag</td>
 * </tr>
 * </table>
 * 
 * <p>User-defined data and control codes are packed as follows:
 * <table border="1" summary="User-defined codes bit packing">
 * <tr>
 *   <th>bits</th>
 *   <th>field</th>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;0 - &nbsp;3</code></td>
 *   <td>binary group 1</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;4 - &nbsp;7</code></td>
 *   <td>binary group 2</td>
 * </tr>
 * <tr>
 *   <td><code>&nbsp;8 - 11</code></td>
 *   <td>binary group 3</td>
 * </tr>
 * <tr>
 *   <td>{@code 12 - 15}</td>
 *   <td>binary group 4</td>
 * </tr>
 * <tr>
 *   <td>{@code 12 - 15}</td>
 *   <td>binary group 5</td>
 * </tr>
 * <tr>
 *   <td>{@code 16 - 19}</td>
 *   <td>binary group 6</td>
 * </tr>
 * <tr>
 *   <td>{@code 24 - 27}</td>
 *   <td>binary group 7</td>
 * </tr>
 * <tr>
 *   <td>{@code 28 - 31}</td>
 *   <td>binary group 8</td>
 * </tr>
 * </table>
 * 
 * @since OpenEXR-JNI 2.1
 */
public class TimeCode implements Cloneable {
    
    /** Bit packing variants */
    public static enum Packing {
        /** Packing for 60-field television */
        TV60,
        /** Packing for 50-field television */
        TV50,
        /** Packing for 24-frame film */
        FILM24
    }

    public int timeAndFlags;
    public int userData;
    
    public TimeCode() {}
    
    public TimeCode(TimeCode other) {
        this.timeAndFlags = other.timeAndFlags;
        this.userData = other.userData;
    }

    public TimeCode(int timeAndFlags, int userData) {
        this.timeAndFlags = timeAndFlags;
        this.userData = userData;
    }
    
    private static int getBitField(int value, int minBit, int maxBit) {
        int shift = minBit;
        int mask = (~(~0 << (maxBit - minBit + 1)) << minBit);
        return (value & mask) >>> shift;
    }
    
    private static int setBitField(int value, int minBit,int maxBit, int field){
        int shift = minBit;
        int mask = (~(~0 << (maxBit - minBit + 1)) << minBit);
        value = ((value & ~mask) | ((field << shift) & mask));
        return value;
    }
    
    private static int bcdToBinary(int bcd) {
        return ((bcd & 0x0f) + 10 * ((bcd >>> 4) & 0x0f));
    }
    
    private static int binaryToBcd(int binary) {
        int units = binary % 10;
        int tens  = (binary / 10) % 10;
        return units | (tens << 4);
    }
        
    
    
    public int getHours() {
        return bcdToBinary(getBitField(timeAndFlags, 24, 29));
    }
    
    public void setHours(int value) {
        if (value < 0 || value > 23) {
            throw new IllegalArgumentException("Cannot set hours field in "
                    + "time code. New value is out of range: " + value);
        }
        timeAndFlags = setBitField(timeAndFlags, 24, 29, binaryToBcd(value));
    }
    
    public int getMinutes() {
        return bcdToBinary(getBitField(timeAndFlags, 16, 22));
    }
    
    public void setMinutes(int value) {
        if (value < 0 || value > 59) {
            throw new IllegalArgumentException("Cannot set minutes field in "
                    + "time code. New value is out of range: " + value);
        }
        timeAndFlags = setBitField(timeAndFlags, 16, 22, binaryToBcd(value));
    }
    
    public int getSeconds() {
        return bcdToBinary(getBitField(timeAndFlags, 8, 14));
    }
    
    public void setSeconds(int value) {
        if (value < 0 || value > 59) {
            throw new IllegalArgumentException("Cannot set seconds field in "
                    + "time code. New value is out of range: " + value);
        }
        timeAndFlags = setBitField(timeAndFlags, 8, 14, binaryToBcd(value));
    }
    
    public int getFrame() {
        return bcdToBinary(getBitField(timeAndFlags, 0, 5));
    }
    
    public void setFrame(int value) {
        if (value < 0 || value > 59) {
            throw new IllegalArgumentException("Cannot set frame field in "
                    + "time code. New value is out of range: " + value);
        }
        timeAndFlags = setBitField(timeAndFlags, 0, 5, binaryToBcd(value));
    }
    
    public boolean hasDropFrame() {
        return getBitField(timeAndFlags, 6, 6) != 0;
    }
    
    public void setDropFrame(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 6, 6, value ? 1 : 0);
    }
    
    public boolean hasColorFrame() {
        return getBitField(timeAndFlags, 7, 7) != 0;
    }
    
    public void setColorFrame(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 7, 7, value ? 1 : 0);
    }
    
    public boolean hasFieldPhase() {
        return getBitField(timeAndFlags, 15, 15) != 0;
    }
    
    public void setFieldPhase(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 15, 15, value ? 1 : 0);
    }
    
    public boolean hasBgf0() {
        return getBitField(timeAndFlags, 23, 23) != 0;
    }
    
    public void setBgf0(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 23, 23, value ? 1 : 0);
    }
    
    public boolean hasBgf1() {
        return getBitField(timeAndFlags, 30, 30) != 0;
    }
    
    public void setBgf1(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 30, 30, value ? 1 : 0);
    }
    
    public boolean hasBgf2() {
        return getBitField(timeAndFlags, 31, 31) != 0;
    }
    
    public void setBgf2(boolean value) {
        timeAndFlags = setBitField(timeAndFlags, 31, 31, value ? 1 : 0);
    }
    
    public int getBinaryGroup(int group) {
        if (group < 1 || group > 8) {
            throw new IndexOutOfBoundsException("Cannot extract binary group " +
                    "from time code user data. Group number is out of range: " +
                    group);
        }
        int minBit = 4 * (group - 1);
        int maxBit = minBit + 3;
        return getBitField(this.userData, minBit, maxBit);
    }
    
    public void setBinaryGroup(int group, int value) {
        if (group < 1 || group > 8) {
            throw new IndexOutOfBoundsException("Cannot set binary group " +
                    "to time code user data. Group number is out of range: " +
                    group);
        }
        if (value < 0 || value > 15) {
            throw new IllegalArgumentException("Binary group value " +
                    "out of range: " + value);
        }
        int minBit = 4 * (group - 1);
        int maxBit = minBit + 3;
        userData = setBitField(userData, minBit, maxBit, value);
    }
    
    public int getTimeAndFlags() {
        return getTimeAndFlags(Packing.TV60);
    }
    
    public int getTimeAndFlags(Packing packing) {
        switch(packing) {
            case TV60:
                return this.timeAndFlags;
            case FILM24:
                return this.timeAndFlags & ~((1 << 6) | (1 << 7));
            case TV50:
                int t = this.timeAndFlags;
                
                t &=~((1 << 6) | (1 << 15) | (1 << 23) | (1 << 30) | (1 << 31));
                
                t |= (hasBgf0() ? 1 : 0) << 15;
                t |= (hasBgf2() ? 1 : 0) << 23;
                t |= (hasBgf1() ? 1 : 0) << 30;
                t |= (hasFieldPhase() ? 1 : 0) << 31;

                return t;
            default:
                throw new IllegalArgumentException(
                        "Invalid packing: " + packing);
        }
    }
    
    public void setTimeAndFlags(int value) {
        setTimeAndFlags(value, Packing.TV60);
    }
    
    public void setTimeAndFlags(int value, Packing packing) {
        switch(packing) {
            case TV60:
                this.timeAndFlags = value;
                break;
            case FILM24:
                this.timeAndFlags = value & ~((1 << 6) | (1 << 7));
                break;
            case TV50:
                this.timeAndFlags = value &
                    ~((1 << 6) | (1 << 15) | (1 << 23) | (1 << 30) | (1 << 31));
                if ((value & (1 << 15)) != 0) {
                    setBgf0(true);
                }
                if ((value & (1 << 23)) != 0) {
                    setBgf2(true);
                }
                if ((value & (1 << 30)) != 0) {
                    setBgf1(true);
                }
                if ((value & (1 << 31)) != 0) {
                    setFieldPhase(true);
                } 
                break;
            default:
                throw new IllegalArgumentException(
                        "Invalid packing: " + packing);
        }
    }
    
    public int getUserData() {
        return userData;
    }
    
    public void setUserData(int value) {
        this.userData = value;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final TimeCode other = (TimeCode) obj;
        if (this.timeAndFlags != other.timeAndFlags) {
            return false;
        }
        if (this.userData != other.userData) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 5;
        hash = 29 * hash + this.timeAndFlags;
        hash = 29 * hash + this.userData;
        return hash;
    }

    @Override
    public String toString() {
        return String.format("%02d:%02d:%02d:%02d %#x",
            getHours(), getMinutes(), getSeconds(), getFrame(), getUserData());
    }

    @Override
    public TimeCode clone() {
        try {
            return (TimeCode) super.clone();
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

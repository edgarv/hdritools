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

import java.util.Collections;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.Set;
import java.util.TreeMap;

/**
 * A {@code FrameBuffer} is a collection of name-slice mappings using to
 * describe the data layout when reading or writing a file.
 * 
 * @see Slice
 */
public class FrameBuffer implements Iterable<Entry<String, Slice>> {
    
    // Slices have to be ordered by name
    private final TreeMap<String, Slice> map = new TreeMap<>();
    
    /**
     * Returns a read-only iterator over the existing slices.
     * 
     * <p>The elements are ordered by slice name. The returned entries
     * represent snapshots of the slices at the time they were produced.
     * 
     * @return Returns a read-only iterator over the existing slices.
     */
    @Override
    public Iterator<Entry<String, Slice>> iterator() {
        return Collections.unmodifiableSet(map.entrySet()).iterator();
    }
    
    /**
     * Returns a read-only {@link Set} view of the slice names contained
     * in this frame buffer. The set's iterator returns the slice names in
     * ascending order.
     * 
     * @return a read-only set view of the slice names contained
     *         in this frame buffer.
     */
    public Set<String> attributeNameSet() {
        return Collections.unmodifiableSet(map.keySet());
    }
    
    /**
     * Returns a read-only {@link Set} view of the name-slice mappings 
     * contained in this frame buffer. The set's iterator returns the entries in 
     * ascending name order.
     * 
     * @return a read-only set view of the name-slice mappings 
     *         contained in this frame buffer.
     */
    public Set<Entry<String, Slice>> entrySet() {
        return Collections.unmodifiableSet(map.entrySet());
    }
    
    public void insert(String name, Slice slice) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Slice attribute name cannot be "
                    + " an empty string.");
        } else if (slice == null) {
            throw new IllegalArgumentException("null slice");
        }
        map.put(name, slice);
    }
    
    // Does not throw
    public Slice findSlice(String name) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Slice attribute name cannot be "
                    + " an empty string.");
        }
        Slice slice = map.get(name);
        return slice;
    }
    
    // Throw ArgExc if not found
    public Slice getSlice(String name) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Slice attribute name cannot be "
                    + " an empty string.");
        }
        Slice slice = map.get(name);
        if (slice == null) {
            throw new IllegalArgumentException("Slice not found: " + name);
        }
        return slice;
    }
    
    /**
     * Returns the number of name-slice mappings in this frame buffer.
     * @return the number of name-slice mappings in this frame buffer
     */
    public int size() {
        return map.size();
    }
    
    /**
     * Returns {@code true} if this frame buffer contains no name-slice mappings.
     * @return {@code true} if this frame buffer contains no name-slice mappings
     */
    public boolean isEmpty() {
        return map.isEmpty();
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 41 * hash + this.map.hashCode();
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
        final FrameBuffer other = (FrameBuffer) obj;
        if (!Objects.equals(this.map, other.map)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "FrameBuffer{" + map + '}';
    }
    
}

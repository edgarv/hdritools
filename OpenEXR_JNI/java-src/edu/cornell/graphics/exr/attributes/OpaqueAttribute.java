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

package edu.cornell.graphics.exr.attributes;

import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;
import java.util.Arrays;

/**
 * Holds the value of an attribute whose type name was not recognized by
 * the reading program. {@code OpaqueAttribute} objects can be read from an
 * image file, copied and written back to another image file, but their values
 * are inaccessible.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class OpaqueAttribute implements Attribute {
    
    private final String name;
    private byte[] data;
    
    public OpaqueAttribute(String name) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Null or empy name");
        }
        this.name = name;
    }

    @Override
    public String typeName() {
        return name;
    }
    
    /**
     * Returns the size in bytes of the data held by this opaque attribute.
     * @return the size in bytes of the data held by this opaque attribute
     */
    public int getSize() {
        if (data != null) {
            return data.length;
        } else {
            return 0;
        }
    }

    @Override
    public int hashCode() {
        int hash = 7;
        hash = 97 * hash + (this.name != null ? this.name.hashCode() : 0);
        hash = 97 * hash + Arrays.hashCode(this.data);
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
        final OpaqueAttribute other = (OpaqueAttribute) obj;
        if ((this.name == null) ? (other.name != null) : !this.name.equals(other.name)) {
            return false;
        }
        if (!Arrays.equals(this.data, other.data)) {
            return false;
        }
        return true;
    }

    @Override
    public String toString() {
        return "OpaqueAttribute{" + name + ", data[" + data.length + "]}";
    }

    @Override
    public void readValueFrom(XdrInput input, int size, int version)
            throws EXRIOException {
        data = new byte[size];
        input.readFully(data);
    }

    @Override
    public void writeValueTo(XdrOutput output, int ver) throws EXRIOException {
        output.writeByteArray(data);
    }

    @Override
    public OpaqueAttribute clone() {
        try {
            OpaqueAttribute attr = (OpaqueAttribute) super.clone();
            // Strings are immutable, thus it's OK just to copy the reference,
            // but we need a deep copy of the data
            attr.data = data.clone();
            return attr;
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

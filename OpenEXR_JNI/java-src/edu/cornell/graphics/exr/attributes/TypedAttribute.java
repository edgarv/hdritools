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
import java.io.IOException;

// TODO: Add documentation
public abstract class TypedAttribute<T> implements Attribute {
    
    protected T value;  
    
    protected static <E extends Enum<E>> E valueOf(int ordinal, E[] values) {
        for (E e : values) {
            if (e.ordinal() == ordinal) {
                return e;
            }
        }
        return null;
    }
    
    protected static <E extends Enum<E>> E checkedValueOf(int ordinal,
            E[] values) throws EXRIOException {
        E e = valueOf(ordinal, values);
        if (e != null) {
            return e;
        } else {
            throw new EXRIOException("Not a valid ordinal: " + ordinal);
        }
    }
    
    protected static void checkSize(int expected, int actual)
            throws EXRIOException {
        if (expected != actual) {
            throw new EXRIOException(String.format(
                    "Expected size %d, actual %d", expected, actual));
        }
    }
    
    /**
     * Set the value of this attribute by reading from the given input buffer.
     * The {@code version} parameters is the 4-byte integer
     * following the magic number at the beginning of an OpenEXR file with the
     * file version and feature flags.
     * 
     * <p>The default implementation throws an
     * {@code UnsupportedOperationException}</p>
     * 
     * @param input data input from which the value will be read.
     * @param version file version and flags as provided in the OpenEXR file.
     * @throws EXRIOException if there is an error in the file format.
     * @throws IOException if there is an I/O error.
     */
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        throw new UnsupportedOperationException("Not implemented");
    }

    @Override
    public void readValueFrom(XdrInput input, int size,
            int version) throws EXRIOException, IOException {
        final long p0  = input.position();
        readValueFrom(input, version);
        final int actualCount = (int) (input.position() - p0);
        checkSize(size, actualCount);
    }
    
    /**
     * Writes the value of this attribute into the given output buffer. This
     * method is used by the default implementation of
     * {@link #writeValueTo(XdrOutput, int) }.
     * 
     * <p>The default implementation throws an
     * {@code UnsupportedOperationException}</p>
     * 
     * @param output data output into which the value will be written.
     * @throws EXRIOException if there is an I/O error.
     */
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        throw new UnsupportedOperationException("Not implemented");
    }
    
    @Override
    public void writeValueTo(XdrOutput output, int version)
            throws EXRIOException {
        writeValueTo(output);
    }
    
    public T getValue() {
        return value;
    }
    
    public void setValue(T value) {
        if (value == null) {
            throw new IllegalArgumentException("The value cannot be null");
        }
        this.value = value;
    }

    @Override
    public String toString() {
        return typeName() + '{' + value + '}';
    }

    @Override
    public int hashCode() {
        final String name = typeName();
        int hash = 3;
        hash = 83 * hash + (this.value != null ? this.value.hashCode() : 0);
        hash = 83 * hash + (name != null ? name.hashCode() : 0);
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
        final TypedAttribute<T> other = (TypedAttribute<T>) obj;
        if (this.value != other.value && 
           (this.value == null || !this.value.equals(other.value))) {
            return false;
        }
        final String name = typeName();
        final String otherName = other.typeName();
        if ((name == null) ? (otherName != null) : !name.equals(otherName)) {
            return false;
        }
        return true;
    }
    
    /**
     * Clones the value, required by {@link #clone() }. Most of the times
     * instances should create a deep copy of their value.
     * 
     * @return a clone of the value.
     */
    protected abstract T cloneValue();

    @Override
    public final TypedAttribute clone() {
        try {
            TypedAttribute<T> attr = (TypedAttribute<T>) super.clone();
            attr.value = this.cloneValue();
            return attr;
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

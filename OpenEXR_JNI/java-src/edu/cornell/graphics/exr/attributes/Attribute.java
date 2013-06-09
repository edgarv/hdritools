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

/**
 * Interface for the attributes which may be included in the header of OpenEXR
 * files.
 * 
 * <p>Attributes provide a unique string identifying their concrete type via
 * {@code typeName()}. They can create deep copies of themselves as specified
 * in the contract of {@code Cloneable}. The functions 
 * {@code writeValueTo(XdrOutput, int)} and
 * {@code readValueFrom(XdrInput, int, int)} implement the serialization 
 * routines for writing and reading and attribute from the OpenEXR file header.
 * 
 * @since OpenEXR-JNI 2.1
 */
public interface Attribute extends Cloneable {
    
    /** Interface for objects which are registered with the attribute system */
    public static interface AttributeCreator {
        Attribute newInstance();
    }
    
    /** Interface for providers of attributes creators */
    public static interface AttributeCreatorProvider {
        /**
         * Creates a new instance of an {@code AttributeCreator} which can
         * create instances of this attribute type.
         * 
         * @return a new instance of an {@code AttributeCreator}.
         */
        AttributeCreator newCreator();
    }
    
    /**
     * Get this attribute's type name as it appears in an OpenEXR header. The
     * type name is case sensitive.
     * 
     * @return the type name as it appears in the OpenEXR header.
     */
    String typeName();
    
    /**
     * Set the value of this attribute by reading from the given input buffer.
     * The {@code size} parameter contains the size in bytes specified in the
     * header for the attribute's value; {@code version} is the 4-byte integer
     * following the magic number at the beginning of an OpenEXR file with the
     * file version and feature flags.
     * 
     * @param input data input from which the value will be read.
     * @param size amount of bytes to be read according to the header.
     * @param version file version and flags as provided in the OpenEXR file.
     * @throws EXRIOException if there is an error in the file format or
     *         an I/O error.
     */
    void readValueFrom(XdrInput input, int size, int version)
            throws EXRIOException;
    
    /**
     * Writes the value of this attribute into the given output buffer.
     * The {@code version} parameter is the 4-byte integer following the
     * magic number at the beginning of an OpenEXR file with the
     * file version and feature flags.
     * 
     * @param output data output into which the value will be written.
     * @param version file version and flags as provided in the OpenEXR file.
     * @throws EXRIOException if there is an I/O error.
     */
    void writeValueTo(XdrOutput output, int version) throws EXRIOException;

    /**
     * Creates a deep-copy of this attribute.
     * @return a deep-copy of this attribute.
     */
    Attribute clone();
    
}

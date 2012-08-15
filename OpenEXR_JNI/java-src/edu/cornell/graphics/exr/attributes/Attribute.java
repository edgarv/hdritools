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
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public interface Attribute {
    
    /** Interface for objects which are registered with the attribute system */
    public static interface AttributeCreator {
        Attribute newInstance();
    }
    
    /** Interface for providers of attributes creators */
    public static interface AttributeCreatorProvider {
        /**
         * Creates a new instance of an {@code AttributeCreator} which can create
         * instances of this attribute type.
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
     * @throws EXRIOException if there is an error in the file format.
     * @throws IOException if there is an I/O error.
     */
    void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException;
}

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

import edu.cornell.graphics.exr.Compression;
import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.io.XdrInput;
import java.io.IOException;

// TODO: Add documentation
public class CompressionAttribute extends TypedAttribute<Compression> {

    public CompressionAttribute() {}
    
    public CompressionAttribute(Compression compression) {
        setValue(compression);
    }

    @Override
    public String typeName() {
        return "compression";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        int ordinal = input.readUnsignedByte();
        Compression c = checkedValueOf(ordinal, Compression.values());
        setValue(c);
    }

    @Override
    protected Compression cloneValue() {
        return value;
    }
    
}

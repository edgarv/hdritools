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

// TODO: Add documentation
public class StringAttribute extends TypedAttribute<String> {

    @Override
    public String typeName() {
        return "string";
    }

    @Override
    public void readValueFrom(XdrInput input, int size, int version)
            throws EXRIOException {
        String s = input.readUTF8(size);
        setValue(s);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final String s = getValue();
        output.writeUTF8(s);
    }

    @Override
    protected String cloneValue() {
        // Strings are immutable
        return value;
    }
}

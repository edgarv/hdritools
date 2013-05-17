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
public class FloatAttribute extends TypedAttribute<Float> {

    public FloatAttribute() {}
    
    public FloatAttribute(float f) {
        setValue(f);
    }

    @Override
    public String typeName() {
        return "float";
    }

    @Override
    protected void readValueFrom(EXRBufferedDataInput input, int version)
            throws EXRIOException, IOException {
        float f = input.readFloat();
        setValue(f);
    }

    @Override
    protected Float cloneValue() {
        return new Float(value);
    }
    
}

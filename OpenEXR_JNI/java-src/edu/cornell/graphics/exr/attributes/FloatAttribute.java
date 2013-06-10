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
 * A {@code TypedAttribute} subclass holding a {@code Float} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class FloatAttribute extends TypedAttribute<Float> {
    
    public FloatAttribute() {
        // empty
    }
    
    public FloatAttribute(float f) {
        super(f);
    }

    @Override
    public String typeName() {
        return "float";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        float f = input.readFloat();
        setValue(f);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        float v = getValue();
        output.writeFloat(v);
    }

    @Override
    protected Float cloneValue() {
        // Float objects are immutable
        return value;
    }
    
}

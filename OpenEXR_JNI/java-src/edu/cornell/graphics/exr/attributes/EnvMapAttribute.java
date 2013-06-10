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
import edu.cornell.graphics.exr.EnvMap;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding an {@code EnvMap} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class EnvMapAttribute extends TypedAttribute<EnvMap> {
    
    public EnvMapAttribute() {
        // empty
    }
    
    public EnvMapAttribute(EnvMap value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "envmap";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        int ordinal = input.readUnsignedByte();
        EnvMap v = checkedValueOf(ordinal, EnvMap.class);
        setValue(v);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        int ordinal = getValue().ordinal();
        output.writeUnsignedByte(ordinal);
    }

    @Override
    protected EnvMap cloneValue() {
        return value;
    }
    
}

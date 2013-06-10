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
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code Vector2<Double>} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class V2dAttribute extends TypedAttribute<Vector2<Double>> {
    
    public V2dAttribute() {
        // empty
    }
    
    public V2dAttribute(Vector2<Double> value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "v2d";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Vector2<Double> v = new Vector2<>();
        v.x = input.readDouble();
        v.y = input.readDouble();
        setValue(v);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Vector2<Double> v = getValue();
        output.writeDouble(v.x);
        output.writeDouble(v.y);
    }

    @Override
    protected Vector2<Double> cloneValue() {
        return new Vector2<>(value);
    }
    
}

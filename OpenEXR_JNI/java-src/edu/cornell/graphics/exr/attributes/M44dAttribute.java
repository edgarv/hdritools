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
import edu.cornell.graphics.exr.ilmbaseto.Matrix44;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code Matrix44<Double>} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class M44dAttribute extends TypedAttribute<Matrix44<Double>> {
    
    public M44dAttribute() {
        // empty
    }
    
    public M44dAttribute(Matrix44<Double> value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "m44d";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Matrix44<Double> m = new Matrix44<>();
                
        m.m00 = input.readDouble();
        m.m01 = input.readDouble();
        m.m02 = input.readDouble();
        m.m03 = input.readDouble();

        m.m10 = input.readDouble();
        m.m11 = input.readDouble();
        m.m12 = input.readDouble();
        m.m13 = input.readDouble();

        m.m20 = input.readDouble();
        m.m21 = input.readDouble();
        m.m22 = input.readDouble();
        m.m23 = input.readDouble();

        m.m30 = input.readDouble();
        m.m31 = input.readDouble();
        m.m32 = input.readDouble();
        m.m33 = input.readDouble();

        setValue(m);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Matrix44<Double> m = getValue();
        
        output.writeDouble(m.m00);
        output.writeDouble(m.m01);
        output.writeDouble(m.m02);
        output.writeDouble(m.m03);
        
        output.writeDouble(m.m10);
        output.writeDouble(m.m11);
        output.writeDouble(m.m12);
        output.writeDouble(m.m13);
        
        output.writeDouble(m.m20);
        output.writeDouble(m.m21);
        output.writeDouble(m.m22);
        output.writeDouble(m.m23);
        
        output.writeDouble(m.m30);
        output.writeDouble(m.m31);
        output.writeDouble(m.m32);
        output.writeDouble(m.m33);
    }

    @Override
    protected Matrix44<Double> cloneValue() {
        return new Matrix44<>(value);
    }
    
}

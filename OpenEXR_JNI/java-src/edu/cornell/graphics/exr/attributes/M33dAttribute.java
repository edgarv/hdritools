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
import edu.cornell.graphics.exr.ilmbaseto.Matrix33;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code Matrix33<Double>} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class M33dAttribute extends TypedAttribute<Matrix33<Double>> {
    
    public M33dAttribute() {
        // empty
    }
    
    public M33dAttribute(Matrix33<Double> value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "m33d";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Matrix33<Double> m = new Matrix33<>();
                
        m.m00 = input.readDouble();
        m.m01 = input.readDouble();
        m.m02 = input.readDouble();

        m.m10 = input.readDouble();
        m.m11 = input.readDouble();
        m.m12 = input.readDouble();

        m.m20 = input.readDouble();
        m.m21 = input.readDouble();
        m.m22 = input.readDouble();

        setValue(m);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Matrix33<Double> m = getValue();
        
        output.writeDouble(m.m00);
        output.writeDouble(m.m01);
        output.writeDouble(m.m02);
        
        output.writeDouble(m.m10);
        output.writeDouble(m.m11);
        output.writeDouble(m.m12);
        
        output.writeDouble(m.m20);
        output.writeDouble(m.m21);
        output.writeDouble(m.m22);
    }

    @Override
    protected Matrix33<Double> cloneValue() {
        return new Matrix33<>(value);
    }
    
}

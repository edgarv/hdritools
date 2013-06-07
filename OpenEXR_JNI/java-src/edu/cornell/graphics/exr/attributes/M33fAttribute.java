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

// TODO: Add documentation
public class M33fAttribute extends TypedAttribute<Matrix33<Float>> {

    @Override
    public String typeName() {
        return "m33f";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Matrix33<Float> m = new Matrix33<>();
                
        m.m00 = input.readFloat();
        m.m01 = input.readFloat();
        m.m02 = input.readFloat();

        m.m10 = input.readFloat();
        m.m11 = input.readFloat();
        m.m12 = input.readFloat();

        m.m20 = input.readFloat();
        m.m21 = input.readFloat();
        m.m22 = input.readFloat();

        setValue(m);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Matrix33<Float> m = getValue();
        
        output.writeFloat(m.m00);
        output.writeFloat(m.m01);
        output.writeFloat(m.m02);
        
        output.writeFloat(m.m10);
        output.writeFloat(m.m11);
        output.writeFloat(m.m12);
        
        output.writeFloat(m.m20);
        output.writeFloat(m.m21);
        output.writeFloat(m.m22);
    }

    @Override
    protected Matrix33<Float> cloneValue() {
        return new Matrix33<>(value);
    }
    
}

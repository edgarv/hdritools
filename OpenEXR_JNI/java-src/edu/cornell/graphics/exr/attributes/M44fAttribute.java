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
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class M44fAttribute extends TypedAttribute<Matrix44<Float>> {

    @Override
    public String typeName() {
        return "m44f";
    }

    @Override
    protected void readValueFrom(EXRBufferedDataInput input, int version)
            throws EXRIOException, IOException {
        Matrix44<Float> m = new Matrix44<Float>();
        
        m.m00 = input.readFloat();
        m.m01 = input.readFloat();
        m.m02 = input.readFloat();
        m.m03 = input.readFloat();

        m.m10 = input.readFloat();
        m.m11 = input.readFloat();
        m.m12 = input.readFloat();
        m.m13 = input.readFloat();

        m.m20 = input.readFloat();
        m.m21 = input.readFloat();
        m.m22 = input.readFloat();
        m.m23 = input.readFloat();

        m.m30 = input.readFloat();
        m.m31 = input.readFloat();
        m.m32 = input.readFloat();
        m.m33 = input.readFloat();

        setValue(m);
    }

    @Override
    protected Matrix44<Float> cloneValue() {
        return new Matrix44<Float>(value);
    }
    
}

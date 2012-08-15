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
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class M33dAttribute extends TypedAttribute<Matrix33<Double>> {

    @Override
    public String typeName() {
        return "m33d";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(9*8, size);
        Matrix33<Double> m = new Matrix33<Double>();
                
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
}

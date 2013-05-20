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
import edu.cornell.graphics.exr.ilmbaseto.Vector3;
import edu.cornell.graphics.exr.io.XdrInput;
import java.io.IOException;

// TODO: Add documentation
public class V3iAttribute extends TypedAttribute<Vector3<Integer>> {

    @Override
    public String typeName() {
        return "v3i";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        Vector3<Integer> v = new Vector3<Integer>();
        v.x = input.readInt();
        v.y = input.readInt();
        v.z = input.readInt();
        setValue(v);
    }
    
    @Override
    protected Vector3<Integer> cloneValue() {
        return new Vector3<Integer>(value);
    }
    
}

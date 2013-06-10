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
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code Vector3<Integer>} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class V3iAttribute extends TypedAttribute<Vector3<Integer>> {
    
    public V3iAttribute() {
        // empty
    }
    
    public V3iAttribute(Vector3<Integer> value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "v3i";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Vector3<Integer> v = new Vector3<>();
        v.x = input.readInt();
        v.y = input.readInt();
        v.z = input.readInt();
        setValue(v);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Vector3<Integer> v = getValue();
        output.writeInt(v.x);
        output.writeInt(v.y);
        output.writeInt(v.z);
    }
    
    @Override
    protected Vector3<Integer> cloneValue() {
        return new Vector3<>(value);
    }
    
}

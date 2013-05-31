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
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;
import java.io.IOException;

// TODO: Add documentation
public class Box2fAttribute extends TypedAttribute<Box2<Float>> {

    @Override
    public String typeName() {
        return "box2f";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        Box2<Float> box = new Box2<>();
        box.xMin = input.readFloat();
        box.yMin = input.readFloat();
        box.xMax = input.readFloat();
        box.yMax = input.readFloat();
        setValue(box);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Box2<Float> box = getValue();
        output.writeFloat(box.xMin);
        output.writeFloat(box.yMin);
        output.writeFloat(box.xMax);
        output.writeFloat(box.yMax);
    }

    @Override
    protected Box2<Float> cloneValue() {
        return new Box2<>(value);
    }

}

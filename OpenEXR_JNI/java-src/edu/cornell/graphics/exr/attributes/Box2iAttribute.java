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
import java.io.IOException;

// TODO: Add documentation
public class Box2iAttribute extends TypedAttribute<Box2<Integer>> {

    public Box2iAttribute() {}
    
    public Box2iAttribute(Box2<Integer> box2) {
        setValue(box2);
    }

    @Override
    public String typeName() {
        return "box2i";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        Box2<Integer> box = new Box2<>();
        box.xMin = input.readInt();
        box.yMin = input.readInt();
        box.xMax = input.readInt();
        box.yMax = input.readInt();
        setValue(box);
    }

    @Override
    protected Box2<Integer> cloneValue() {
        return new Box2<>(value);
    }
    
}

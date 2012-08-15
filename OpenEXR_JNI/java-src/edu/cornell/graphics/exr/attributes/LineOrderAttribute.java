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
import edu.cornell.graphics.exr.LineOrder;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class LineOrderAttribute extends TypedAttribute<LineOrder> {

    public LineOrderAttribute() {}
    
    public LineOrderAttribute(LineOrder lineOrder) {
        setValue(lineOrder);
    }

    @Override
    public String typeName() {
        return "lineOrder";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(1, size);
        int ordinal = input.readUnsignedByte();
        LineOrder lo = checkedValueOf(ordinal, LineOrder.values());
        setValue(lo);
    }
}

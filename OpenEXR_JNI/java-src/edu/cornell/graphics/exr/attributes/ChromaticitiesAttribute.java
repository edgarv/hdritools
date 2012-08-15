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

import edu.cornell.graphics.exr.Chromaticities;
import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO Add documentation
public class ChromaticitiesAttribute extends TypedAttribute<Chromaticities> {

    @Override
    public String typeName() {
        return "chromaticities";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(8*4, size);
        Chromaticities c = new Chromaticities();
        c.redX   = input.readFloat();
        c.redY   = input.readFloat();
        c.greenX = input.readFloat();
        c.greenY = input.readFloat();
        c.blueX  = input.readFloat();
        c.blueY  = input.readFloat();
        c.whiteX = input.readFloat();
        c.whiteY = input.readFloat();
        setValue(c);
    }
}

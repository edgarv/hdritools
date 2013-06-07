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
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

// TODO Add documentation
public class ChromaticitiesAttribute extends TypedAttribute<Chromaticities> {

    @Override
    public String typeName() {
        return "chromaticities";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
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

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Chromaticities c = this.getValue();
        output.writeFloat(c.redX);
        output.writeFloat(c.redY);
        output.writeFloat(c.greenX);
        output.writeFloat(c.greenY);
        output.writeFloat(c.blueX);
        output.writeFloat(c.blueY);
        output.writeFloat(c.whiteX);
        output.writeFloat(c.whiteY);
    }

    @Override
    protected Chromaticities cloneValue() {
        return new Chromaticities(value);
    }
    
}

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
import edu.cornell.graphics.exr.TileDescription;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class TileDescriptionAttribute extends TypedAttribute<TileDescription> {

    @Override
    public String typeName() {
        return "tiledesc";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(4+4+1, size);
        TileDescription t = new TileDescription();
        t.xSize = input.readInt();
        if (t.xSize < 0) {
            throw new IOException("xSize overflow: " + t.xSize);
        }
        t.ySize = input.readInt();
        if (t.ySize < 0) {
            throw new IOException("ySize overflow: " + t.ySize);
        }

        int modeRaw = input.readUnsignedByte();
        int levelModeOrd    = modeRaw & 0xf;
        int roundingModeOrd = modeRaw >>> 4;
        t.mode    = checkedValueOf(levelModeOrd,
                TileDescription.LevelMode.values());
        t.roundingMode = checkedValueOf(roundingModeOrd,
                TileDescription.RoundingMode.values());
        
        setValue(t);
    }
}

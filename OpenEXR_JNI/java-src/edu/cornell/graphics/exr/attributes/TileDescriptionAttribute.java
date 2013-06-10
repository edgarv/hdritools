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
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code TileDescription} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class TileDescriptionAttribute extends
        TypedAttribute<TileDescription> {
    
    public TileDescriptionAttribute() {
        // empty
    }
    
    public TileDescriptionAttribute(TileDescription value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "tiledesc";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        TileDescription t = new TileDescription();
        t.xSize = input.readInt();
        if (t.xSize < 0) {
            throw new EXRIOException("xSize overflow: " + t.xSize);
        }
        t.ySize = input.readInt();
        if (t.ySize < 0) {
            throw new EXRIOException("ySize overflow: " + t.ySize);
        }

        int modeRaw = input.readUnsignedByte();
        int levelModeOrd    = modeRaw & 0xf;
        int roundingModeOrd = modeRaw >>> 4;
        t.mode    = checkedValueOf(levelModeOrd,
                TileDescription.LevelMode.class);
        t.roundingMode = checkedValueOf(roundingModeOrd,
                TileDescription.RoundingMode.class);
        
        setValue(t);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final TileDescription t = getValue();
        
        if (t.xSize < 0) {
            throw new EXRIOException("xSize overflow: " + t.xSize);
        }
        if (t.ySize < 0) {
            throw new EXRIOException("ySize overflow: " + t.ySize);
        }
        output.writeInt(t.xSize);
        output.writeInt(t.ySize);
        
        int tmp = t.mode.ordinal() | (t.roundingMode.ordinal() << 4);
        output.writeUnsignedByte(tmp);
    }

    @Override
    protected TileDescription cloneValue() {
        return value.clone();
    }
    
}

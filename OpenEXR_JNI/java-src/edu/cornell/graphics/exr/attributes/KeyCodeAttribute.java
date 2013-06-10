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
import edu.cornell.graphics.exr.KeyCode;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code KeyCode} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class KeyCodeAttribute extends TypedAttribute<KeyCode> {
    
    public KeyCodeAttribute() {
        // empty
    }
    
    public KeyCodeAttribute(KeyCode value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "keycode";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        KeyCode k = new KeyCode();
        k.filmMfcCode   = input.readInt();
        k.filmType      = input.readInt();
        k.prefix        = input.readInt();
        k.count         = input.readInt();
        k.perfOffset    = input.readInt();
        k.perfsPerFrame = input.readInt();
        k.perfsPerCount = input.readInt();
        setValue(k);
    }
    
    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final KeyCode k = getValue();
        output.writeInt(k.filmMfcCode);
        output.writeInt(k.filmType);
        output.writeInt(k.prefix);
        output.writeInt(k.count);
        output.writeInt(k.perfOffset);
        output.writeInt(k.perfsPerFrame);
        output.writeInt(k.perfsPerCount);
    }

    @Override
    protected KeyCode cloneValue() {
        return new KeyCode(value);
    }
    
}

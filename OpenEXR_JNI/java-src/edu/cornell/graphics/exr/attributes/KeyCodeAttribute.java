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
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class KeyCodeAttribute extends TypedAttribute<KeyCode> {

    @Override
    public String typeName() {
        return "keycode";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(7*4, size);
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
    protected KeyCode cloneValue() {
        return new KeyCode(value);
    }
    
}

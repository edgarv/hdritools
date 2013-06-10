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
import edu.cornell.graphics.exr.TimeCode;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code TimeCode} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class TimeCodeAttribute extends TypedAttribute<TimeCode> {
    
    public TimeCodeAttribute() {
        // empty
    }
    
    public TimeCodeAttribute(TimeCode value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "timecode";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        TimeCode t = new TimeCode();
        t.timeAndFlags = input.readInt();
        t.userData     = input.readInt();
        setValue(t);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final TimeCode t = getValue();
        output.writeInt(t.timeAndFlags);
        output.writeInt(t.userData);
    }

    @Override
    protected TimeCode cloneValue() {
        return value.clone();
    }
    
}

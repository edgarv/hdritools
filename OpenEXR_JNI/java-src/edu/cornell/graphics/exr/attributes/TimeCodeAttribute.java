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
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import java.io.IOException;

// TODO: Add documentation
public class TimeCodeAttribute extends TypedAttribute<TimeCode> {

    @Override
    public String typeName() {
        return "timecode";
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
            throws EXRIOException, IOException {
        checkSize(4+4, size);
        TimeCode r = new TimeCode();
        r.timeAndFlags = input.readInt();
        r.userData     = input.readInt();
        setValue(r);
    }

    @Override
    protected TimeCode cloneValue() {
        return value.clone();
    }
    
}

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
import edu.cornell.graphics.exr.PreviewImage;
import edu.cornell.graphics.exr.io.XdrInput;
import java.io.IOException;

// TODO: Add documentation
public class PreviewImageAttribute extends TypedAttribute<PreviewImage> {

    @Override
    public String typeName() {
        return "preview";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException, IOException {
        PreviewImage p = new PreviewImage();
        p.width  = input.readInt();
        if (p.width < 0) {
            throw new IOException("width overflow: " + p.width);
        }
        p.height = input.readInt();
        if (p.height < 0) {
            throw new IOException("height overflow: " + p.height);
        }
        
        int pixelsLen = 4 * p.width * p.height;
        p.pixelData = new byte[pixelsLen];
        input.readFully(p.pixelData);
        
        setValue(p);
    }

    @Override
    protected PreviewImage cloneValue() {
        return value.clone();
    }
    
}

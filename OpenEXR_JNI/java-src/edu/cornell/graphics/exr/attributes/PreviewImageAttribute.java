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
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code PreviewImage} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class PreviewImageAttribute extends TypedAttribute<PreviewImage> {
    
    public PreviewImageAttribute() {
        // empty
    }
    
    public PreviewImageAttribute(PreviewImage value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "preview";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        PreviewImage p = new PreviewImage();
        p.width  = input.readInt();
        if (p.width < 0) {
            throw new EXRIOException("width overflow: " + p.width);
        }
        p.height = input.readInt();
        if (p.height < 0) {
            throw new EXRIOException("height overflow: " + p.height);
        }
        
        int pixelsLen = 4 * p.width * p.height;
        p.pixelData = new byte[pixelsLen];
        input.readFully(p.pixelData);
        
        setValue(p);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final PreviewImage p = new PreviewImage();
        
        if (p.width < 0) {
            throw new EXRIOException("width overflow: " + p.width);
        } else if (p.height < 0) {
            throw new EXRIOException("height overflow: " + p.height);
        }
        output.writeInt(p.width);
        output.writeInt(p.height);
        
        final int numPixels = p.width * p.height;
        if (p.pixelData != null) {
            output.writeByteArray(p.pixelData, 0, numPixels);            
        } else {
            output.pad(numPixels);
        }
    }

    @Override
    protected PreviewImage cloneValue() {
        return value.clone();
    }
    
}

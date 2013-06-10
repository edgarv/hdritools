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
import java.util.Objects;

/**
 * A {@code TypedAttribute} subclass holding a {@code PreviewImage} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public final class PreviewImageAttribute extends TypedAttribute<PreviewImage> {
    
    public PreviewImageAttribute() {
        super();
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
        final int width  = input.readInt();
        if (width < 0) {
            throw new EXRIOException("width overflow: " + width);
        }
        final int height = input.readInt();
        if (height < 0) {
            throw new EXRIOException("height overflow: " + height);
        }
        PreviewImage p = new PreviewImage(width, height);
        assert p.getPixelData().length == (4 * width * height);
        input.readFully(p.getPixelData());
        setValue(p);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final PreviewImage p = Objects.requireNonNull(getValue());
        assert p.getPixelData().length == (4 * p.getWidth() * p.getHeight());
        output.writeInt(p.getWidth());
        output.writeInt(p.getHeight());
        output.writeByteArray(p.getPixelData());
    }

    @Override
    protected PreviewImage cloneValue() {
        return value.clone();
    }
    
}

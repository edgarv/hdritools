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
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

/**
 * A {@code TypedAttribute} subclass holding a {@code Double} value.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class DoubleAttribute extends TypedAttribute<Double> {
    
    public DoubleAttribute() {
        // empty
    }
    
    public DoubleAttribute(double value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "double";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        double v = input.readDouble();
        setValue(v);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        double v = getValue();
        output.writeDouble(v);
    }

    @Override
    protected Double cloneValue() {
        // Double objects are immutable
        return value;
    }
    
}

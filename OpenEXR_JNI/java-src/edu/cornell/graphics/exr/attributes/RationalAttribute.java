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
import edu.cornell.graphics.exr.Rational;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;

// TODO: Add documentation
public class RationalAttribute extends TypedAttribute<Rational> {
    
    public RationalAttribute() {
        // empty
    }
    
    public RationalAttribute(Rational value) {
        super(value);
    }

    @Override
    public String typeName() {
        return "rational";
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
            throws EXRIOException {
        Rational r = new Rational();
        r.n = input.readInt();
        r.d = input.readInt();
        setValue(r);
    }

    @Override
    protected void writeValueTo(XdrOutput output) throws EXRIOException {
        final Rational r = getValue();
        output.writeInt(r.n);
        output.writeInt(r.d);
    }

    @Override
    protected Rational cloneValue() {
        return new Rational(value);
    }
    
}

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

package edu.cornell.graphics.exr;

// TODO: Add documentation
public class EXRTypeException extends RuntimeException {

    public EXRTypeException(Throwable cause) {
        super(cause);
    }

    public EXRTypeException(String message, Throwable cause) {
        super(message, cause);
    }

    public EXRTypeException(String message) {
        super(message);
    }

    public EXRTypeException() {
    }
    
}

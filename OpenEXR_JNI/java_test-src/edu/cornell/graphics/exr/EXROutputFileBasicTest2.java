/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2013 Program of Computer Graphics, Cornell University

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

import edu.cornell.graphics.exr.io.EXRFileOutputStream;
import java.io.File;

/**
 * Initial test suite for EXROutputFile using Java {@code EXROutputStream}
 * instances from the native code.
 */
public class EXROutputFileBasicTest2 extends EXROutputFileBasicTest1 {

    @Override
    protected EXROutputFile createOutputFile(File file, Header header) 
            throws EXRIOException {
        EXROutputFile output = new EXROutputFile(
                new EXRFileOutputStream(file.toPath()), header, true);
        return output;
    }
    
}

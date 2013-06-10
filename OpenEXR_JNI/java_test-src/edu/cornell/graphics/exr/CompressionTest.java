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

import static org.junit.Assert.assertEquals;
import org.junit.Test;

public class CompressionTest {
    
    /**
     * Check that the ordinals match the native code
     */
    @Test
    public void testOrdinals() {
        System.out.println("Compression ordinals");
        assertEquals(0L, Compression.NONE.ordinal());
        assertEquals(1L, Compression.RLE.ordinal());
        assertEquals(2L, Compression.ZIPS.ordinal());
        assertEquals(3L, Compression.ZIP.ordinal());
        assertEquals(4L, Compression.PIZ.ordinal());
        assertEquals(5L, Compression.PXR24.ordinal());
        assertEquals(6L, Compression.B44.ordinal());
        assertEquals(7L, Compression.B44A.ordinal());
    }

}

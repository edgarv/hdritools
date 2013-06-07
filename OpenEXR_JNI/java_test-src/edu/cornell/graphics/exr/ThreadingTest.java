/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

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

import static org.junit.Assert.*;
import org.junit.Test;

public class ThreadingTest {
    
    @Test
    public void testGlobalThreadCount() {
        System.out.println("globalThreadCount");
        final int countOrig = Threading.globalThreadCount();
        assertTrue(countOrig >= 0);
        int count = Runtime.getRuntime().availableProcessors();
        Threading.setGlobalThreadCount(count);
        int currentCount = Threading.globalThreadCount();
        assertEquals(count, currentCount);
    }
    
    @Test(expected=RuntimeException.class)
    public void testSetGlobalThreadCountBad() {
        System.out.println("setGlobalThreadCountBad");
        Threading.setGlobalThreadCount(-1);
        fail("This should have failed");
    }

}

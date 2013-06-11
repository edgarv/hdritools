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
package edu.cornell.graphics.exr.io;

import edu.cornell.graphics.exr.Header;
import edu.cornell.graphics.exr.TestUtil;
import java.io.IOException;
import java.net.URISyntaxException;
import java.nio.file.Path;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Basic test using a simple, known file.
 */
public class InputFileInfoBasicTest {
    
    private final static String RES_FILENMAME =
            "edu/cornell/graphics/exr/resources/test-piz-rgb.exr";
    
    private static InputFileInfo instance;
    
    public InputFileInfoBasicTest() {
        // Nothing to do
    }
    
    @BeforeClass
    public static void setUpClass() throws IOException, URISyntaxException {
        // As a basic test, they all use the same instance
        Path path = TestUtil.getResourcePath(RES_FILENMAME);
        instance = new InputFileInfo(path);
    }
    
    @Before
    public void setUp() {
        assertNotNull(instance);
    }

    /**
     * Test of isTiled method, of class InputFileInfo.
     */
    @Test
    public void testIsTiled() {
        System.out.println("isTiled");
        boolean expResult = false;
        boolean result = instance.isTiled();
        assertEquals(expResult, result);
    }

    /**
     * Test of getVersion method, of class InputFileInfo.
     */
    @Test
    public void testGetVersion() {
        System.out.println("getVersion");
        int expResult = 2;
        int result = instance.getVersion();
        assertEquals(expResult, result);
    }

    /**
     * Test of getHeader method, of class InputFileInfo.
     */
    @Test
    public void testGetHeader() {
        System.out.println("getHeader");
        Header header = instance.getHeader();
        assertNotNull(header);
        assertFalse(header.hasTileDescription());
    }

    /**
     * Test of getFilename method, of class InputFileInfo.
     */
    @Test
    public void testGetFilename() {
        System.out.println("getFilename");
        String result = instance.getFilename();
        assertNotNull(result);
    }

    /**
     * Test of isComplete method, of class InputFileInfo.
     */
    @Test
    public void testIsComplete() {
        System.out.println("isComplete");
        boolean expResult = true;
        boolean result = instance.isComplete();
        assertEquals(expResult, result);
    }
}

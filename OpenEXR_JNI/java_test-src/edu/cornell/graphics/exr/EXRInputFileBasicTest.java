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

import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import java.io.IOException;
import java.net.URISyntaxException;
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.nio.file.Paths;
import org.junit.After;
import org.junit.AfterClass;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Tests reading using the general scanline-oriented interface with a known file
 */
public class EXRInputFileBasicTest {
    
    private final static String RES_FILENMAME =
            "edu/cornell/graphics/exr/resources/test-piz-rgb.exr";
    
    private static EXRInputFile file;
    
    public EXRInputFileBasicTest() {
        // empty
    }
    
    @BeforeClass
    public static void setUpClass() throws URISyntaxException, IOException {
        // As a basic test, they all use the same instance
        java.net.URL url = ClassLoader.getSystemResource(RES_FILENMAME);
        if (url == null) {
            fail("Could not open source resource: " + RES_FILENMAME);
        }
        final Path path = Paths.get(url.toURI());
        file = new EXRInputFile(path);
    }
    
    @AfterClass
    public static void tearDownClass() throws EXRIOException {
        if (file != null) {
            file.close();
        }
    }
    
    @Before
    public void setUp() {
        assertNotNull(file);
        assertTrue(file.isOpen());
    }
    
    @After
    public void tearDown() {
        // empty
    }

    /**
     * Test of getHeader method, of class EXRInputFile.
     */
    @Test
    public void testGetHeader() {
        System.out.println("getHeader");
        Header header = file.getHeader();
        assertNotNull(header);
        
        Box2<Integer> dwRef = new Box2<>(0, 0, 1023, 31);
        Vector2<Float> centerRef = new Vector2<>(0.0f, 0.0f);
        
        assertEquals(dwRef, header.getDataWindow());
        assertEquals(dwRef, header.getDisplayWindow());
        assertEquals(Compression.PIZ, header.getCompression());
        assertEquals(LineOrder.INCREASING_Y, header.getLineOrder());
        assertEquals(new Float(1.0f), header.getPixelAspectRatio());
        assertEquals(new Float(1.0f), header.getScreenWindowWidth());
        assertEquals(centerRef, header.getScreenWindowCenter());
        assertFalse(header.hasTileDescription());
        
        ChannelList channels = header.getChannels();
        assertNotNull(channels);
        assertEquals(3L, channels.size());
        ChannelList channelsReference = new ChannelList();
        channelsReference.insert("R", new Channel());
        channelsReference.insert("G", new Channel());
        channelsReference.insert("B", new Channel());
        assertEquals(channelsReference, channels);
    }

    /**
     * Test of getVersion method, of class EXRInputFile.
     */
    @Test
    public void testGetVersion() {
        int expResult = 2;
        int result = file.getVersion();
        assertEquals(expResult, result);
    }

    /**
     * Test of isComplete method, of class EXRInputFile.
     */
    @Test
    public void testIsComplete() {
        System.out.println("isComplete");
        boolean result = file.isComplete();
        assertTrue(result);
    }

    /**
     * Read pixels into interleaved float channels, whole image
     */
    @Test
    public void testReadPixels1() {
        System.out.println("readPixels - interleaved float32 RGB");
        FrameBuffer frameBuffer = new FrameBuffer();
        final int width  = 1024;
        final int height = 32;
        final int numChannels = 3;
        final PixelType type = PixelType.FLOAT;
        final int pixelSize = numChannels * type.byteSize();
        final int xStride = pixelSize;
        final int yStride = width * pixelSize;
        
        java.nio.ByteBuffer buffer = java.nio.ByteBuffer.allocateDirect(
                width * height * pixelSize);
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        frameBuffer.insert("R", Slice.build().buffer(buffer)
                .baseOffset(0*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("G", Slice.build().buffer(buffer)
                .baseOffset(1*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("B", Slice.build().buffer(buffer)
                .baseOffset(2*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());

        // In this file all pixels are the same
        file.setFrameBuffer(frameBuffer);
        file.readPixels(0, height-1);
        for (int i = 0; i < width*height; ++i) {
            float r = buffer.getFloat();
            float g = buffer.getFloat();
            float b = buffer.getFloat();
            assertEquals(0.75, r, 0.0);
            assertEquals(0.50, g, 0.0);
            assertEquals(0.25, b, 0.0);
        }
    }
    
    /**
     * Read pixels into interleaved half channels, whole image
     */
    @Test
    public void testReadPixels2() {
        System.out.println("readPixels - interleaved half RGB");
        FrameBuffer frameBuffer = new FrameBuffer();
        final int width  = 1024;
        final int height = 32;
        final int numChannels = 3;
        final PixelType type = PixelType.HALF;
        final int pixelSize = numChannels * type.byteSize();
        final int xStride = pixelSize;
        final int yStride = width * pixelSize;
        
        java.nio.ByteBuffer buffer = java.nio.ByteBuffer.allocateDirect(
                width * height * pixelSize);
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        frameBuffer.insert("R", Slice.build().buffer(buffer)
                .baseOffset(0*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("G", Slice.build().buffer(buffer)
                .baseOffset(1*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("B", Slice.build().buffer(buffer)
                .baseOffset(2*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());

        // In this file all pixels are the same
        file.setFrameBuffer(frameBuffer);
        file.readPixels(0, height-1);
        short rRef = Half.floatToShortBits(0.75f);
        short gRef = Half.floatToShortBits(0.50f);
        short bRef = Half.floatToShortBits(0.25f);
        for (int i = 0; i < width*height; ++i) {
            short r = buffer.getShort();
            short g = buffer.getShort();
            short b = buffer.getShort();
            assertEquals(rRef, r);
            assertEquals(gRef, g);
            assertEquals(bRef, b);
        }
    }
    
    /**
     * Read pixels into interleaved half channels, single scan line
     */
    @Test
    public void testReadPixels3() {
        System.out.println("readPixels - interleaved half RGB, one scanline");
        FrameBuffer frameBuffer = new FrameBuffer();
        final int width  = 1024;
        final int height = 32;
        final int numChannels = 3;
        final PixelType type = PixelType.HALF;
        final int pixelSize = numChannels * type.byteSize();
        final int xStride = pixelSize;
        final int yStride = 0;
        
        java.nio.ByteBuffer buffer = java.nio.ByteBuffer.allocateDirect(
                width * 1 * pixelSize);
        buffer.order(ByteOrder.LITTLE_ENDIAN);

        frameBuffer.insert("R", Slice.build().buffer(buffer)
                .baseOffset(0*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("G", Slice.build().buffer(buffer)
                .baseOffset(1*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());
        frameBuffer.insert("B", Slice.build().buffer(buffer)
                .baseOffset(2*type.byteSize())
                .xStride(xStride).yStride(yStride)
                .pixelType(type).get());

        // In this file all pixels are the same
        file.setFrameBuffer(frameBuffer);
        short rRef = Half.floatToShortBits(0.75f);
        short gRef = Half.floatToShortBits(0.50f);
        short bRef = Half.floatToShortBits(0.25f);
        for (int s = 0; s < height; ++s) {
            file.readPixels(s);
            for (int i = 0; i < width; ++i) {
                short r = buffer.getShort();
                short g = buffer.getShort();
                short b = buffer.getShort();
                assertEquals(rRef, r);
                assertEquals(gRef, g);
                assertEquals(bRef, b);
            }
            buffer.rewind();
        }
    }

}

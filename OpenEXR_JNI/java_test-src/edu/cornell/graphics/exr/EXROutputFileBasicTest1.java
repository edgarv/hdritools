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

import edu.cornell.graphics.exr.attributes.CompressionAttribute;
import edu.cornell.graphics.exr.attributes.IntegerAttribute;
import edu.cornell.graphics.exr.attributes.StringAttribute;
import edu.cornell.graphics.exr.io.InputFileInfo;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import jp.ac.hiroshima_u.sci.math.saito.tinymt.TinyMT32;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.Test;

/**
 * Initial test suite for EXROutputFile using the native {@code Imf::OStream}
 * instances.
 */
public class EXROutputFileBasicTest1 {
    
    protected TinyMT32 rnd = TinyMT32.getDefault();
    
    /** Creates a temporary EXR file to be deleted on exit */
    protected static File createTempFile() throws IOException {
        File tmpFile = File.createTempFile("EXROutputFileTest", ".exr");
        tmpFile.deleteOnExit();
        return tmpFile;
    }
    
    protected void insertDummyAttributes(Header header) {
        IntegerAttribute attrInt = new IntegerAttribute();
        StringAttribute  attrStr = new StringAttribute();
        attrInt.setValue(rnd.nextInt());
        header.insert("foo", attrInt);
        attrInt.setValue(rnd.nextInt());
        header.insert("bar", attrInt);
        attrStr.setValue(String.format("%#x %#018x %g",
                rnd.nextLong(), rnd.nextLong(), rnd.nextFloat()));
        header.insert("toto", attrStr);
        attrStr.setValue("test1");
        header.insert("testStr", attrStr);
    }
    
    /**
     * Returns a new {@code EXROutputFile} using the native {@code Imf::OStream}
     * @param header full header of the new file to create
     * @return a new {@code EXROutputFile} using the native {@code Imf::OStream}
     * @throws IOException if there is an I/O error
     */
    protected EXROutputFile createOutputFile(File file, Header header) 
            throws IOException {
        EXROutputFile out = new EXROutputFile(file.toPath(), header);
        return out;
    }
    
    @Before
    public void setUp() {
        rnd.setSeed(new int[]{
            0xcecbcf71, 0xa0ef3280, 0x17a10cc4, 0x5607f153, 0xb3c01f08,
            0x407c9bec, 0x7dcdfb49, 0x7cf3942f, 0x8c1a7387, 0xc29da230,
            0x1b9e55a9, 0xea982777, 0x60c8b3c8, 0x9235484c, 0xb6772595,
            0xe64fcbeb, 0xe36db9f7, 0x878c4eae, 0xd74f72a3, 0x3e47ef25});
    }
    
    /**
     * Test writing a header-only file
     */
    @Test
    public void testHeaderOnly() throws Exception {
        System.out.println("headerOnly");
        Header header = new Header(640, 480);
        insertDummyAttributes(header);
        File tmpFile = createTempFile();
        try (EXROutputFile out = createOutputFile(tmpFile, header)) {
            System.out.printf("  Wrote header-only file %s%n", tmpFile);
            assertEquals(0L, out.currentScanLine());
        }
        
        InputFileInfo info = new InputFileInfo(tmpFile);
        assertEquals(header, info.getHeader());
        System.out.println("  Checked the written header.");
    }
    
    /**
     * Write RGB32F file, read back into RGB32F frame buffer
     */
    @Test
    public void testRGB32F_RGB32F() throws Exception {
        System.out.println("RGB32F -> RGB32F");
        final int width  = 640;
        final int height = 480;
        final PixelType pixelType = PixelType.FLOAT;
        final int numChannels = 3;
        final int elemSize    = pixelType.byteSize();
        final int pixelSize   = elemSize * numChannels;
        final int numPixels   = width * height;
        Header header = new Header(width, height);
        insertDummyAttributes(header);
        ChannelList channels = header.getChannels();
        channels.insert("R", new Channel(pixelType));
        channels.insert("G", new Channel(pixelType));
        channels.insert("B", new Channel(pixelType));
        
        // Random interleaved RGB32F pixels
        ByteBuffer pixels = ByteBuffer.allocateDirect(pixelSize * numPixels);
        pixels.order(ByteOrder.LITTLE_ENDIAN);
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
            }
        }
        pixels.flip();
        final int pixelsHash = pixels.hashCode();
        
        // Build the frame buffer
        FrameBuffer frameBuffer = new FrameBuffer();
        final int yStride = width * pixelSize;
        frameBuffer.insert("R", Slice.build()
                .baseOffset(0).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        
        final File tmpFile = createTempFile();
        try (EXROutputFile out = createOutputFile(tmpFile, header)) {
            assertEquals(0L, out.currentScanLine());
            out.setFrameBuffer(frameBuffer);
            out.writePixels(height);
            assertEquals(height, out.currentScanLine());
            System.out.printf("  Wrote RGB32F file %s%n", tmpFile);
            assertEquals(pixelsHash, pixels.hashCode());
        }
        System.out.printf("  File size: %d%n", Files.size(tmpFile.toPath()));
        pixels.rewind();
        
        // Read back the file
        ByteBuffer pixelsCopy = ByteBuffer.allocateDirect(pixelSize*numPixels);
        pixelsCopy.order(ByteOrder.LITTLE_ENDIAN);
        frameBuffer.getSlice("R").buffer = pixelsCopy;
        frameBuffer.getSlice("G").buffer = pixelsCopy;
        frameBuffer.getSlice("B").buffer = pixelsCopy;
        try (EXRInputFile input = new EXRInputFile(tmpFile.toPath())) {
            assertEquals(header, input.getHeader());
            input.setFrameBuffer(frameBuffer);
            input.readPixels(0, height-1);
            assertEquals(0L, pixels.position());
            assertEquals(pixels.capacity(), pixels.remaining());
            System.out.println("  Read back the file...");
        }
        
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                float rExpected = pixels.getFloat();
                float gExpected = pixels.getFloat();
                float bExpected = pixels.getFloat();
                float r = pixelsCopy.getFloat();
                float g = pixelsCopy.getFloat();
                float b = pixelsCopy.getFloat();
                
                String message = String.format("Pixel (%d,%d)", w,h);
                assertEquals(message, rExpected, r, 0.0);
                assertEquals(message, gExpected, g, 0.0);
                assertEquals(message, bExpected, b, 0.0);
            }
        }
        System.out.println("  All pixels successfully validated!");
    }
    
    /**
     * Write RGB32F file, read back into RGB16F frame buffer
     */
    @Test
    public void testRGB32F_RGB16F() throws Exception {
        System.out.println("RGB32F -> RGB16F");
        final int width  = 1280;
        final int height = 720;
        final int numPixels   = width * height;
        final int numChannels = 3;
        PixelType pixelType = PixelType.FLOAT;
        int elemSize        = pixelType.byteSize();
        int pixelSize       = elemSize * numChannels;
        Header header = new Header(width, height);
        header.getTypedAttribute("compression",
                CompressionAttribute.class).setValue(Compression.ZIP);
        insertDummyAttributes(header);
        ChannelList channels = header.getChannels();
        channels.insert("R", new Channel(pixelType));
        channels.insert("G", new Channel(pixelType));
        channels.insert("B", new Channel(pixelType));
        
        // Random interleaved RGB32F pixels
        ByteBuffer pixels = ByteBuffer.allocateDirect(pixelSize * numPixels);
        pixels.order(ByteOrder.LITTLE_ENDIAN);
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
            }
        }
        pixels.flip();
        final int pixelsHash = pixels.hashCode();
        
        // Build the frame buffer
        FrameBuffer frameBuffer = new FrameBuffer();
        int yStride = width * pixelSize;
        frameBuffer.insert("R", Slice.build()
                .baseOffset(0).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        
        final File tmpFile = createTempFile();
        try (EXROutputFile out = createOutputFile(tmpFile, header)) {
            assertEquals(0L, out.currentScanLine());
            out.setFrameBuffer(frameBuffer);
            out.writePixels(height);
            assertEquals(height, out.currentScanLine());
            System.out.printf("  Wrote RGB32F file %s%n", tmpFile);
            assertEquals(pixelsHash, pixels.hashCode());
        }
        System.out.printf("  File size: %d%n", Files.size(tmpFile.toPath()));
        pixels.rewind();
        
        // Read back the file into half pixels
        pixelType = PixelType.HALF;
        elemSize  = pixelType.byteSize();
        pixelSize = elemSize * numChannels;
        yStride   = width * pixelSize;
        ByteBuffer pixelsCopy = ByteBuffer.allocateDirect(pixelSize*numPixels);
        pixelsCopy.order(ByteOrder.LITTLE_ENDIAN);
        FrameBuffer frameBufferHalf = new FrameBuffer();
        frameBufferHalf.insert("R", Slice.build()
                .baseOffset(0).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBufferHalf.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBufferHalf.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        try (EXRInputFile input = new EXRInputFile(tmpFile.toPath())) {
            assertEquals(header, input.getHeader());
            input.setFrameBuffer(frameBufferHalf);
            input.readPixels(0, height-1);
            assertEquals(0L, pixels.position());
            assertEquals(pixels.capacity(), pixels.remaining());
            System.out.println("  Read back the file...");
        }
        
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                float rExpected = pixels.getFloat();
                float gExpected = pixels.getFloat();
                float bExpected = pixels.getFloat();
                float r = Half.shortBitsToFloat(pixelsCopy.getShort());
                float g = Half.shortBitsToFloat(pixelsCopy.getShort());
                float b = Half.shortBitsToFloat(pixelsCopy.getShort());
                
                String message = String.format("Pixel (%d,%d)", w,h);
                assertEquals(message, rExpected, r, Math.abs(5e-4 * rExpected));
                assertEquals(message, gExpected, g, Math.abs(5e-4 * gExpected));
                assertEquals(message, bExpected, b, Math.abs(5e-4 * bExpected));
            }
        }
        System.out.println("  All pixels successfully validated!");
    }
    
    /**
     * Write RGB32F file, read back into RGBA16F frame buffer
     */
    @Test
    public void testRGB32F_RGBA16F() throws Exception {
        System.out.println("RGB32F -> RGBA16F");
        final int width  = 1280;
        final int height = 720;
        final int numPixels   = width * height;
        int numChannels = 3;
        PixelType pixelType = PixelType.FLOAT;
        int elemSize        = pixelType.byteSize();
        int pixelSize       = elemSize * numChannels;
        Header header = new Header(width, height);
        header.getTypedAttribute("compression",
                CompressionAttribute.class).setValue(Compression.ZIP);
        insertDummyAttributes(header);
        ChannelList channels = header.getChannels();
        channels.insert("R", new Channel(pixelType));
        channels.insert("G", new Channel(pixelType));
        channels.insert("B", new Channel(pixelType));
        
        // Random interleaved RGB32F pixels
        ByteBuffer pixels = ByteBuffer.allocateDirect(pixelSize * numPixels);
        pixels.order(ByteOrder.LITTLE_ENDIAN);
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
                pixels.putFloat(rnd.nextFloat() * (rnd.nextBoolean() ? 1 : -1));
            }
        }
        pixels.flip();
        final int pixelsHash = pixels.hashCode();
        
        // Build the frame buffer
        FrameBuffer frameBuffer = new FrameBuffer();
        int yStride = width * pixelSize;
        frameBuffer.insert("R", Slice.build()
                .baseOffset(0).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        
        final File tmpFile = createTempFile();
        try (EXROutputFile out = createOutputFile(tmpFile, header)) {
            assertEquals(0L, out.currentScanLine());
            out.setFrameBuffer(frameBuffer);
            out.writePixels(height);
            assertEquals(height, out.currentScanLine());
            System.out.printf("  Wrote RGB32F file %s%n", tmpFile);
            assertEquals(pixelsHash, pixels.hashCode());
        }
        System.out.printf("  File size: %d%n", Files.size(tmpFile.toPath()));
        pixels.rewind();
        
        // Read back the file into half pixels
        numChannels = 4;
        pixelType   = PixelType.HALF;
        elemSize    = pixelType.byteSize();
        pixelSize   = elemSize * numChannels;
        yStride     = width * pixelSize;
        ByteBuffer pixelsCopy = ByteBuffer.allocateDirect(pixelSize*numPixels);
        pixelsCopy.order(ByteOrder.LITTLE_ENDIAN);
        FrameBuffer frameBufferHalf = new FrameBuffer();
        frameBufferHalf.insert("R", Slice.build()
                .baseOffset(0).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBufferHalf.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBufferHalf.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBufferHalf.insert("A", Slice.build()
                .baseOffset(3*elemSize).buffer(pixelsCopy).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).fillValue(1.0).get());
        try (EXRInputFile input = new EXRInputFile(tmpFile.toPath())) {
            assertEquals(header, input.getHeader());
            input.setFrameBuffer(frameBufferHalf);
            input.readPixels(0, height-1);
            assertEquals(0L, pixels.position());
            assertEquals(pixels.capacity(), pixels.remaining());
            System.out.println("  Read back the file...");
        }
        
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                float rExpected = pixels.getFloat();
                float gExpected = pixels.getFloat();
                float bExpected = pixels.getFloat();
                float r = Half.shortBitsToFloat(pixelsCopy.getShort());
                float g = Half.shortBitsToFloat(pixelsCopy.getShort());
                float b = Half.shortBitsToFloat(pixelsCopy.getShort());
                float a = Half.shortBitsToFloat(pixelsCopy.getShort());
                
                String message = String.format("Pixel (%d,%d)", w,h);
                assertEquals(message, rExpected, r, Math.abs(5e-4 * rExpected));
                assertEquals(message, gExpected, g, Math.abs(5e-4 * gExpected));
                assertEquals(message, bExpected, b, Math.abs(5e-4 * bExpected));
                assertEquals(message, 1.0,       a, 0.0);
            }
        }
        System.out.println("  All pixels successfully validated!");
    }
    
    /**
     * Write RGBAZ16F file, read back into RGBAZ16F frame buffer
     */
    @Test
    public void testRGBAZ16F_RGBAZ16F() throws Exception {
        System.out.println("RGBAZ16F -> RGBAZ16F");
        final int width  = 1920;
        final int height = 1080;
        final PixelType pixelType = PixelType.HALF;
        final int numChannels = 5;
        final int elemSize    = pixelType.byteSize();
        final int pixelSize   = elemSize * numChannels;
        final int numPixels   = width * height;
        Header header = new Header(width, height);
        insertDummyAttributes(header);
        ChannelList channels = header.getChannels();
        channels.insert("R", new Channel(pixelType));
        channels.insert("G", new Channel(pixelType));
        channels.insert("B", new Channel(pixelType));
        channels.insert("A", new Channel(pixelType));
        channels.insert("Z", new Channel(pixelType));
        
        // Random interleaved RGBAZ16F pixels
        ByteBuffer pixels = ByteBuffer.allocateDirect(pixelSize * numPixels);
        pixels.order(ByteOrder.LITTLE_ENDIAN);
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                float r =  rnd.nextFloat() * Half.MAX_VALUE;
                float g =  rnd.nextFloat() * Half.MAX_VALUE;
                float b =  rnd.nextFloat() * Half.MAX_VALUE;
                float a =  rnd.nextFloat();
                float z = -rnd.nextFloat();
                
                short rH = Half.floatToShortBits(r);
                short gH = Half.floatToShortBits(g);
                short bH = Half.floatToShortBits(b);
                short aH = Half.floatToShortBits(a);
                short zH = Half.floatToShortBits(z);
                
                pixels.putShort(rH);
                pixels.putShort(gH);
                pixels.putShort(bH);
                pixels.putShort(aH);
                pixels.putShort(zH);
            }
        }
        pixels.flip();
        final int pixelsHash = pixels.hashCode();
        
        // Build the frame buffer
        FrameBuffer frameBuffer = new FrameBuffer();
        final int yStride = width * pixelSize;
        frameBuffer.insert("R", Slice.build()
                .baseOffset(0).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("G", Slice.build()
                .baseOffset(elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("B", Slice.build()
                .baseOffset(2*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("A", Slice.build()
                .baseOffset(3*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        frameBuffer.insert("Z", Slice.build()
                .baseOffset(4*elemSize).buffer(pixels).pixelType(pixelType)
                .xStride(pixelSize).yStride(yStride).get());
        
        final File tmpFile = createTempFile();
        try (EXROutputFile out = createOutputFile(tmpFile, header)) {
            assertEquals(0L, out.currentScanLine());
            out.setFrameBuffer(frameBuffer);
            out.writePixels(height);
            assertEquals(height, out.currentScanLine());
            System.out.printf("  Wrote RGB32F file %s%n", tmpFile);
            assertEquals(pixelsHash, pixels.hashCode());
        }
        System.out.printf("  File size: %d%n", Files.size(tmpFile.toPath()));
        pixels.rewind();
        
        // Read back the file
        ByteBuffer pixelsCopy = ByteBuffer.allocateDirect(pixelSize*numPixels);
        pixelsCopy.order(ByteOrder.LITTLE_ENDIAN);
        frameBuffer.getSlice("R").buffer = pixelsCopy;
        frameBuffer.getSlice("G").buffer = pixelsCopy;
        frameBuffer.getSlice("B").buffer = pixelsCopy;
        frameBuffer.getSlice("A").buffer = pixelsCopy;
        frameBuffer.getSlice("Z").buffer = pixelsCopy;
        try (EXRInputFile input = new EXRInputFile(tmpFile.toPath())) {
            assertEquals(header, input.getHeader());
            input.setFrameBuffer(frameBuffer);
            input.readPixels(0, height-1);
            assertEquals(0L, pixels.position());
            assertEquals(pixels.capacity(), pixels.remaining());
            System.out.println("  Read back the file...");
        }
        
        for(int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                Half rExpected = new Half(pixels.getShort());
                Half gExpected = new Half(pixels.getShort());
                Half bExpected = new Half(pixels.getShort());
                Half aExpected = new Half(pixels.getShort());
                Half zExpected = new Half(pixels.getShort());
                
                Half r = new Half(pixelsCopy.getShort());
                Half g = new Half(pixelsCopy.getShort());
                Half b = new Half(pixelsCopy.getShort());
                Half a = new Half(pixelsCopy.getShort());
                Half z = new Half(pixelsCopy.getShort());
                
                String message = String.format("Pixel (%d,%d)", w,h);
                assertEquals(message, rExpected, r);
                assertEquals(message, gExpected, g);
                assertEquals(message, bExpected, b);
                assertEquals(message, aExpected, a);
                assertEquals(message, zExpected, z);
            }
        }
        System.out.println("  All pixels successfully validated!");
    }
    
}

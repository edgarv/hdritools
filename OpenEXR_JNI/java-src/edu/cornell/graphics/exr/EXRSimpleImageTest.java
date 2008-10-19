package edu.cornell.graphics.exr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Random;
import java.util.Map.Entry;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import edu.cornell.graphics.exr.EXRSimpleImage.Channels;


public class EXRSimpleImageTest {
	
	private static final float HALF_MAX = 65504.0f;

	private static ArrayList<Compression> COMPRESSION_LOSSLESS = 
		new ArrayList<Compression>();

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		
		// Stores the proper lossless compressions
		for (Compression c : Compression.values()) {
			if (c.isLossless()) {
				COMPRESSION_LOSSLESS.add(c);
			}
		}
	}
	
	@Before
	public void setUp() throws EXRIOException {
		
	}

	@Test(expected=IllegalArgumentException.class)
	public void testEXRSimpleImageIntIntChannelsEx1() {
		new EXRSimpleImage(0, 0, null);
	}
	@Test(expected=IllegalArgumentException.class)
	public void testEXRSimpleImageIntIntChannelsEx2() {
		new EXRSimpleImage(10, 0, null);
	}
	@Test(expected=IllegalArgumentException.class)
	public void testEXRSimpleImageIntIntChannelsEx3() {
		new EXRSimpleImage(0, 10, null);
	}
	@Test(expected=NullPointerException.class)
	public void testEXRSimpleImageIntIntChannelsEx4() {
		new EXRSimpleImage(10, 10, null);
	}

	@Test(expected=EXRIOException.class)
	public void testEXRSimpleImageFileChannels() throws EXRIOException {
		new EXRSimpleImage(new File("non existent"), Channels.RGB);
	}

	@Test(expected=IllegalArgumentException.class)
	public void testSetBufferEx1() {
		EXRSimpleImage img = new EXRSimpleImage(10,10,Channels.RGB);
		img.setBuffer(null);
	}
	@Test(expected=IllegalArgumentException.class)
	public void testSetBufferEx2() {
		EXRSimpleImage img = new EXRSimpleImage(10,10,Channels.RGB);
		img.setBuffer(new float[10*10*3-1]);
	}
	@Test
	public void testSetBuffer() {
		EXRSimpleImage img;
		float [] buffer;
		
		img = new EXRSimpleImage(640,480,Channels.RGB);
		buffer = new float[640*480*3];
		img.setBuffer(buffer);
		assertSame(buffer, img.getBuffer());
		
		img = new EXRSimpleImage(640,480,Channels.RGBA);
		buffer = new float[640*480*4];
		img.setBuffer(buffer);
		assertSame(buffer, img.getBuffer());
	}

	@Test
	public void testAllocateBuffer() {
		EXRSimpleImage img1 = new EXRSimpleImage(640,480,Channels.RGB);
		assertNull(img1.getBuffer());
		img1.allocateBuffer();
		assertNotNull(img1.getBuffer());
		assertEquals(img1.getBuffer().length, 640*480*3);
		
		EXRSimpleImage img2 = new EXRSimpleImage(640,480,Channels.RGBA);
		assertNull(img2.getBuffer());
		img2.allocateBuffer();
		assertNotNull(img2.getBuffer());
		assertEquals(img2.getBuffer().length, 640*480*4);
	}


	@Test
	public void testGetChannels() {
		EXRSimpleImage img1 = new EXRSimpleImage(640,480,Channels.RGB);
		assertEquals(img1.getChannels(), Channels.RGB);
		EXRSimpleImage img2 = new EXRSimpleImage(640,480,Channels.RGBA);
		assertEquals(img2.getChannels(), Channels.RGBA);
	}

	@Test
	public void testGetNumChannels() {
		EXRSimpleImage img1 = new EXRSimpleImage(640,480,Channels.RGB);
		assertEquals(img1.getNumChannels(), 3);
		EXRSimpleImage img2 = new EXRSimpleImage(640,480,Channels.RGBA);
		assertEquals(img2.getNumChannels(), 4);
	}

	@Test
	public void testGetAttributes() {
		// The default attributes are null
		EXRSimpleImage img1 = new EXRSimpleImage(640,480,Channels.RGB);
		assertNull(img1.getAttributes());
		
		Attributes attributes = new Attributes();
		img1.setAttributes(attributes);
		assertSame(img1.getAttributes(), attributes);
		System.out.println("Default attributes: " + img1.getAttributes());
		
		// By default the attributes have the current date
		assertTrue(img1.getAttributes().getDate() != null);
		Date date = new Date();
		assertTrue("Creation time in the future: " + 
				img1.getAttributes().getDate(),
				date.after(img1.getAttributes().getDate()));
		// Allow a sec difference
		assertTrue("Time difference is greater than 1s.",
				date.getTime() - img1.getAttributes().getDate().getTime() < 1000 );
	}

	@Test
	public void testSetAttributes() {
		Attributes attrib = new Attributes();
		Date date = new Date();
		assertEquals(attrib.getOwner(), System.getProperty("user.name"));
		long deltaTime = date.getTime() - attrib.getDate().getTime();
		assertTrue("Time difference is greater than 1s: " + deltaTime,
				 deltaTime < 1000 );
		assertNull(attrib.getComments());
		
		attrib.setOwner(null);
		attrib.setDate((Date)null);
		assertNull(attrib.getOwner());
		assertNull(attrib.getDate());
		assertNull(attrib.getComments());
		
		attrib.setOwner("foo");
		assertEquals(attrib.getOwner(), "foo");
		attrib.setDate(date);
		deltaTime = date.getTime() - attrib.getDate().getTime();
		assertTrue("Time difference is greater than 1s: " + deltaTime,
				deltaTime < 1000 );
		attrib.setComments("Lorem ipsum");
		assertEquals(attrib.getComments(), "Lorem ipsum");
	}

	@Test
	public void testGetWidthHeight() {
		EXRSimpleImage img1 = new EXRSimpleImage(640,480,Channels.RGB);
		assertEquals(img1.getWidth(), 640);
		assertEquals(img1.getHeight(), 480);
		
	}
	
	
	// Small helper function for getIndex planned failures
	private void testGetIndexBad(EXRSimpleImage img, int x, int y) {
		try {
			img.getIndex(x, y);
			fail("The index (" + x +"," + y + ") was expected to be invalid.");			
		}
		catch(IndexOutOfBoundsException e1) {}
		catch(IllegalArgumentException e2) {}
	}

	@Test
	public void testGetIndex() {
		EXRSimpleImage img = null;
		int nc;
		
		img = new EXRSimpleImage(640,480,Channels.RGB);
		nc  = 3;
		assertEquals(nc, img.getNumChannels());
		assertEquals(img.getNumChannels(), img.getChannels().getNumChannels());
		assertEquals(0, img.getIndex(0, 0));
		assertEquals((640*480-1)*nc, img.getIndex(639,479));
		assertEquals(13*nc, img.getIndex(13, 0));
		assertEquals((640*300+55)*nc, img.getIndex(55, 300));
		
		img = new EXRSimpleImage(640,480,Channels.RGBA);
		nc  = 4;
		assertEquals(nc, img.getNumChannels());
		assertEquals(img.getNumChannels(), img.getChannels().getNumChannels());
		assertEquals(0, img.getIndex(0, 0));
		assertEquals((640*480-1)*nc, img.getIndex(639,479));
		assertEquals(13*nc, img.getIndex(13, 0));
		assertEquals((640*300+55)*nc, img.getIndex(55, 300));
		
		// Tests bad indices
		testGetIndexBad(img, -1, 0);
		testGetIndexBad(img, 0, -1);
		testGetIndexBad(img, -1, -1);
		testGetIndexBad(img, 640, 0);
		testGetIndexBad(img, 0, 480);
		testGetIndexBad(img, 30, 480);
		testGetIndexBad(img, 640, 480);
		testGetIndexBad(img, 0, 4000);
	}
	

	@Test
	public void testGetIndexFast() {
		EXRSimpleImage img = null;
		int nc;
		
		img = new EXRSimpleImage(640,480,Channels.RGB);
		nc  = 3;
		assertEquals(nc, img.getNumChannels());
		assertEquals(img.getNumChannels(), img.getChannels().getNumChannels());
		assertEquals(0, img.getIndexFast(0, 0));
		assertEquals((640*480-1)*nc, img.getIndexFast(639,479));
		assertEquals(13*nc, img.getIndexFast(13, 0));
		assertEquals((640*300+55)*nc, img.getIndexFast(55, 300));
		
		img = new EXRSimpleImage(640,480,Channels.RGBA);
		nc  = 4;
		assertEquals(nc, img.getNumChannels());
		assertEquals(img.getNumChannels(), img.getChannels().getNumChannels());
		assertEquals(0, img.getIndexFast(0, 0));
		assertEquals((640*480-1)*nc, img.getIndexFast(639,479));
		assertEquals(13*nc, img.getIndexFast(13, 0));
		assertEquals((640*300+55)*nc, img.getIndexFast(55, 300));
		
		// Also the invalid ones work here
		img.getIndexFast(-1, 0);
		img.getIndexFast(0, -1);
		img.getIndexFast(-1, -1);
		img.getIndexFast(640, 0);
		img.getIndexFast(0, 480);
		img.getIndexFast(30, 480);
		img.getIndexFast(640, 480);
		img.getIndexFast(0, 4000);
	}

	@Test
	public void testGetPixelIntIntRGB() {
		EXRSimpleImage img = new EXRSimpleImage(10,10, Channels.RGB);
		img.allocateBuffer();
		java.util.Arrays.fill(img.getBuffer(), 0.0f);
		
		final int base = img.getIndex(4, 5);
		float[] pixelOrig = new float[]{1.0f, 2.0f, 3.0f};
		System.arraycopy(pixelOrig, 0, 
				img.getBuffer(), base, pixelOrig.length);
		
		float[] pixel = img.getPixel(4, 5);
		assertArrayEquals(pixelOrig, pixel, 0.0f);
		for (int i = 0; i < img.getBuffer().length; ++i) {
			if (i < base || i > base+2) {
				assertEquals(0.0f, img.getBuffer()[i], 0.0f);
			}
		}
	}
	
	@Test
	public void testGetPixelIntIntRGBA() {
		EXRSimpleImage img = new EXRSimpleImage(10,10, Channels.RGBA);
		img.allocateBuffer();
		java.util.Arrays.fill(img.getBuffer(), 0.0f);
		
		final int base = img.getIndex(3, 8);
		float[] pixelOrig = new float[]{1.0f, 2.0f, 3.0f, 4.0f};
		System.arraycopy(pixelOrig, 0, 
				img.getBuffer(), base, pixelOrig.length);
		
		float[] pixel = img.getPixel(3, 8);
		assertArrayEquals(pixelOrig, pixel, 0.0f);
		for (int i = 0; i < img.getBuffer().length; ++i) {
			if (i < base || i > base+3) {
				assertEquals(0.0f, img.getBuffer()[i], 0.0f);
			}
		}
	}
	
	@Test
	public void testGetPixelEx() {
		
		EXRSimpleImage img = new EXRSimpleImage(512,512,Channels.RGB);
		
		try {
			img.getPixel(0, 0);
			fail("Expected an exception.");
		} catch (IllegalStateException e){}
		
		// Allocate the buffer to avoid the illegal state exception
		img.allocateBuffer();
		
		try {
			img.getPixel(-1, 0);
			fail("Expected an exception.");
		} catch (IndexOutOfBoundsException e) {}
		try {
			img.getPixel(-1, -1);
			fail("Expected an exception.");
		} catch (IndexOutOfBoundsException e) {}
		try {
			img.getPixel(0, 100000);
			fail("Expected an exception.");
		} catch (IndexOutOfBoundsException e) {}
		try {
			img.getPixel(1111111, 1111111);
			fail("Expected an exception.");
		} catch (IndexOutOfBoundsException e) {}
		try {
			img.getPixel(img.getWidth(), img.getHeight());
			fail("Expected an exception.");
		} catch (IndexOutOfBoundsException e) {}
		img.getPixel(img.getWidth()-1, img.getHeight()-1);
	}

	/**
	 * Like the junit assertArrayEquals, but for floating point
	 * arguments
	 * 
	 * @param expected
	 * @param actuals
	 * @param epsilon tolerance to pass: abs(expected[i]-actuals[i]) <= epsilon
	 */
	private static void assertArrayEquals(float[] expected, float[] actuals, float epsilon) {
		if (expected == null || actuals == null) {
			fail("The arrays are null.");
		}
		if (expected.length != actuals.length) {
			fail("The expected lenght was " + expected.length +
				 " but the actual length is " + actuals.length);
		}
		for (int i = 0; i < expected.length; ++i) {
			
			if ((Float.isNaN(expected[i]) != Float.isNaN(actuals[i])) ||
				(Float.isInfinite(expected[i]) != Float.isInfinite(actuals[i])) ||
				Math.abs(expected[i]-actuals[i]) > epsilon) {
				fail("At index=" + i + " expected:" + expected[i] +
					 " was: " + actuals[i]);                                           
			}
		}
		
	}
	
	/** 
	 * Mini function to get the epsilon between a float and its
	 * half representation
	 * 
	 * @param n the number to check
	 * @return <code>abs(n - (float)((half)n)))</code>
	 */
	private static float getHalfEpsilon(float n) {
		
		// Remove the last [23-10=13] bits of the mantissa to
		// reflect the accuracy
		return Math.abs(n - Float.intBitsToFloat(
				Float.floatToIntBits(n) & (~(int)0x1FFF) ));
	}
	
	/**
	 * Also compares all the elements of an array where the actual elements
	 * are actually halfs promoted to float, so each value has a different epsilon
	 * 
	 * @param expected original floating point values
	 * @param actualsHalf actual half values promoted to float
	 */
	private static void assertArrayHalfEquals(float[] expected, float[] actualsHalf) {
		if (expected == null || actualsHalf == null) {
			fail("The arrays are null.");
		}
		if (expected.length != actualsHalf.length) {
			fail("The expected lenght was " + expected.length +
				 " but the actual length is " + actualsHalf.length);
		}
		for (int i = 0; i < expected.length; ++i) {
			
			
			final float epsilon = getHalfEpsilon(expected[i]);
			if ((Float.isNaN(expected[i]) != Float.isNaN(actualsHalf[i])) ||
				(Float.isInfinite(expected[i]) != Float.isInfinite(actualsHalf[i])) ||
				Math.abs(expected[i]-actualsHalf[i]) > epsilon) {
				fail("At index=" + i + " expected:" + expected[i] +
					 " was: " + actualsHalf[i]);                                           
			}
		}
		
	}
	
	
	// Helper function to test the pixels getter/setters
	private void testPixelSetGet(int width, int height, Channels channels) {
		
		System.out.printf("Testing for size %dx%d %s...",
				width, height, channels);
		System.out.flush();
		
		EXRSimpleImage img = new EXRSimpleImage(width, height, channels);
		img.allocateBuffer();
		Random rnd = new Random( (((long)width << 32) | 
				(long)height) ^ channels.getNumChannels());
		
		
		// Set pixels randomly
		HashMap<Long, float[]> pixels = new HashMap<Long, float[]>();
		for(int i = 0; i < 1.5f*width*height; ++i) {
			final int x = rnd.nextInt(width);
			final int y = rnd.nextInt(height);
			float[] px = new float[channels.getNumChannels()];
			for (int j = 0; j < px.length; ++j) {
				px[j] = rnd.nextFloat()*(65000.0f*2.0f)-65000.0f;
			}
			
			long index = ((long)x << 32) | y;
			pixels.put(index, px);
			img.setPixel(x, y, px);
			
			final int component = rnd.nextInt(channels.getNumChannels());
			img.setPixelElement(x, y, component, px[component]);
			assertEquals(px[component], 
					img.getPixelElement(x, y, component), 0.0f);
		}
		
		// Retrieve and check the pixels
		float[] tmpPixel = new float[channels.getNumChannels()];
		for( Entry<Long, float[]> e : pixels.entrySet()) {
			final long index = e.getKey();
			final int x = (int)(index >> 32);
			final int y = (int)(index & 0xFFFFFFFF);
			
			assertArrayEquals(e.getValue(), img.getPixel(x, y), 0.0f);
			img.getPixel(x, y, tmpPixel);
			assertArrayEquals(e.getValue(), tmpPixel, 0.0f);
		}
		
		System.out.println("OK");
	}
	

	@Test
	public void testPixelSetGet() {
		testPixelSetGet(1, 1, Channels.RGB);
		testPixelSetGet(1, 1, Channels.RGBA);
		testPixelSetGet(512, 512, Channels.RGB);
		testPixelSetGet(512, 512, Channels.RGBA);
		testPixelSetGet(1, 1000, Channels.RGB);
		testPixelSetGet(1, 1000, Channels.RGBA);
		testPixelSetGet(1000, 1, Channels.RGB);
		testPixelSetGet(1000, 1, Channels.RGBA);
		testPixelSetGet(640, 480, Channels.RGB);
		testPixelSetGet(640, 480, Channels.RGBA);
	}

	
	// Helper method for testing reading / writing
	private void testWriteRead(int width, int height, Channels channels) throws IOException {
		
		System.out.printf("Testing for size %dx%d %s...%n",
				width, height, channels);
		
		Random rnd = new Random( (((long)width << 32) | 
				(long)height) ^ channels.getNumChannels());
		
		EXRSimpleImage img = new EXRSimpleImage(width, height, channels);
		img.allocateBuffer();
		
		// Which is the opposite channel config?
		Channels otherChannelConfig;
		if (channels.equals(Channels.RGB)) {
			otherChannelConfig = Channels.RGBA;
		}
		else if (channels.equals(Channels.RGBA)) {
			otherChannelConfig = Channels.RGB;
		}
		else {
			throw new IllegalArgumentException(
					"RGB/RGBA Channels: cannot get the complement of  " + channels);
		}
		
		// Random pixel allocation
		float[] px = new float[channels.getNumChannels()];
		for(int x = 0; x < width; ++x) {
			for(int y = 0; y < height; ++y) {
				
				for (int j = 0; j < px.length; ++j) {
					px[j] = rnd.nextFloat()*(HALF_MAX*2.0f)-HALF_MAX;
				}
				img.setPixel(x, y, px);
			}
		}
		
		// First version: no attributes
		assertNull(img.getAttributes());
		
		// Write to disk
		File imgFile1 = File.createTempFile("test", ".exr");
		imgFile1.deleteOnExit();
		Compression compression = 
			COMPRESSION_LOSSLESS.get(rnd.nextInt(COMPRESSION_LOSSLESS.size()));
		System.out.printf("  Writing non-attribute file, %s...", compression);
		System.out.flush();
		img.write(imgFile1, compression);
		System.out.println("OK");
		
		// Read from disk and check
		System.out.printf("  Reading non-attribute file, %s...", channels);
		System.out.flush();
		EXRSimpleImage imgRead = new EXRSimpleImage(imgFile1, channels);
		System.out.println("OK");
		assertNull(imgRead.getAttributes());
		assertEquals(width, imgRead.getWidth());
		assertEquals(height, imgRead.getHeight());
		assertArrayHalfEquals(img.getBuffer(), imgRead.getBuffer());
		
		// Read with the other channel configuration
		System.out.printf("  Reading non-attribute file, %s...", 
				otherChannelConfig);
		System.out.flush();
		imgRead = new EXRSimpleImage(imgFile1, otherChannelConfig);
		System.out.println("OK");
		assertNull(imgRead.getAttributes());
		assertEquals(width, imgRead.getWidth());
		assertEquals(height, imgRead.getHeight());
		// By testing the alternative channel configuration, I already
		// know that the common number of channels is 3 (remember:
		// only RGB and RGBA are supported here)
		for(int x = 0; x < width; ++x) {
			for(int y = 0; y < height; ++y) {
				
				assertEquals(img.getPixelElement(x, y, 0),
						imgRead.getPixelElement(x, y, 0),
						getHalfEpsilon(img.getPixelElement(x, y, 0)) );
				assertEquals(img.getPixelElement(x, y, 1),
						imgRead.getPixelElement(x, y, 1),
						getHalfEpsilon(img.getPixelElement(x, y, 1)) );
				assertEquals(img.getPixelElement(x, y, 2),
						imgRead.getPixelElement(x, y, 2),
						getHalfEpsilon(img.getPixelElement(x, y, 2)) );
			}	
		}
		
		// Add the default attributes
		Attributes attrib = new Attributes();
		img.setAttributes(attrib);
		File imgFile2 = File.createTempFile("test", ".exr");
		imgFile2.deleteOnExit();
		compression =
			COMPRESSION_LOSSLESS.get(rnd.nextInt(COMPRESSION_LOSSLESS.size()));
		System.out.printf("  Writing attribute file 1, %s...", compression);
		System.out.flush();
		img.write(imgFile2, compression);
		System.out.println("OK");
		
		// And readback
		System.out.print("  Reading attribute file 1...");
		imgRead = new EXRSimpleImage(imgFile2, channels);
		System.out.println("OK");
		assertNotNull(imgRead.getAttributes());
		assertEquals(attrib.getOwner(), imgRead.getAttributes().getOwner());
		assertEquals(attrib.getDate(),  imgRead.getAttributes().getDate());
		assertNull(imgRead.getAttributes().getComments());
		assertArrayHalfEquals(img.getBuffer(), imgRead.getBuffer());
		
		// Change the owner and add a comment
		attrib.setOwner("Ego");
		attrib.setComments("Lorem ipsum");
		File imgFile3 = File.createTempFile("test", ".exr");
		imgFile3.deleteOnExit();
		compression =
			COMPRESSION_LOSSLESS.get(rnd.nextInt(COMPRESSION_LOSSLESS.size()));
		System.out.printf("  Writing attribute file 2, %s...", compression);
		System.out.flush();
		img.write(imgFile3, compression);
		System.out.println("OK");
		
		
		// And readback
		System.out.print("  Reading attribute file 2...");
		imgRead = new EXRSimpleImage(imgFile3, channels);
		System.out.println("OK");
		assertNotNull(imgRead.getAttributes());
		assertEquals(attrib.getOwner(), imgRead.getAttributes().getOwner());
		assertEquals(attrib.getDate(),  imgRead.getAttributes().getDate());
		assertEquals(attrib.getComments(), imgRead.getAttributes().getComments());
		assertArrayHalfEquals(img.getBuffer(), imgRead.getBuffer());
		
		System.out.printf("Test %dx%d %s - ALL OK!%n",
				width, height, channels);

	}
	// Simpler, impossible :)
	private void testWriteRead(int width, int height) throws IOException {
		testWriteRead(width, height, Channels.RGB);
		testWriteRead(width, height, Channels.RGBA);
	}
	
	@Test
	public void testWriteRead() throws IOException {
		
		testWriteRead(1, 1);
		testWriteRead(512, 512);
		testWriteRead(1, 1000);
		testWriteRead(1000, 1);
		testWriteRead(640, 480);
	}
	
	@Test(expected=EXRIOException.class)
	public void testReadInvalid() throws EXRIOException {
		new EXRSimpleImage("++Invalid filename++", Channels.RGB);
	}
	
	@Test(expected=EXRIOException.class)
	public void testWriteInvalid() throws EXRIOException {
		EXRSimpleImage img = new EXRSimpleImage(64,64, Channels.RGB);
		img.allocateBuffer();
		img.write("\\\\Invalid\\network\\share\test.exr", Compression.NONE);
	}

}

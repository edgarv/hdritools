package edu.cornell.graphics.exr;

import java.io.File;
import java.io.Serializable;


/**
 * <p>Simple base class which represents either RGB or RGBA images
 * whose pixels are stored in the half format. This interface
 * receives and returns 32-bit floating point values: the half
 * representation is internal to the exr files.</p>
 * 
 * <p>This class provides relatively low level access to the
 * underlying structure so that it can easily adapt to various
 * circumstances where other class will actually interact with the user.</p>
 * 
 * @author edgar
 *
 */
public class EXRSimpleImage implements Serializable {


	/**
	 * 
	 */
	private static final long serialVersionUID = -6156770241349949295L;

	/** Default name of the native library */
	private final static String JNI_LIBNAME = "openexrjni"; 

	/**
	 * As a simple image the only supported channel configurations
	 * are RGB and RGBA.
	 */
	public static enum Channels {
		/**
		 * Interleaved RGB (Red-Green-Blue).
		 */
		RGB(3),
		
		/**
		 * Interleaved RGBA (Red-Green-Blue-Alpha).
		 */
		RGBA(4);
		
		private final int numChannels;
		
		Channels(int numChannels) {
			this.numChannels = numChannels;
		}
		
		/** Returns the number of floating point channels */
		public int getNumChannels() {
			return numChannels;
		}
	}
	
	/**
	 * A simple transfer object for reading OpenEXR files
	 */
	private static class OpenEXRTo {
		
		Attributes attrib;
		int width;
		int height;
		float[] buffer;
	}
	
	// Loads the native JNI bindings if they are not already loaded 
	static {
		// The first time it might fail because the library is not loaded
		try {
			if (getNativeVersion() != serialVersionUID) {
				throw new IllegalStateException("Incompatible versions.");
			}
		}
		catch (UnsatisfiedLinkError e) {
			// If it's not loaded, load it and try again
			System.loadLibrary(JNI_LIBNAME);
			if (getNativeVersion() != serialVersionUID) {
				throw new IllegalStateException("Incompatible versions.");
			}
		}
		
		// At this point the library must be loaded and marked as
		// compatible; init the cache & set the multi-threading options
		initCache();
		final int numProcs = Runtime.getRuntime().availableProcessors();
		if (numProcs > 1) {
			setNumWorkingThreads(numProcs);
		}
		
	}
	
	/** Additional attributes of the file */
	private Attributes attrib;
	
	/** The width of the image contained */
	private final int width;
	
	/** The height of the image contained */
	private final int height;
	
	/** Interleaved array with all the data */
	private float[] buffer;
	
	/** Channels configuration */
	private final Channels channels;
	
	/** Number of channels, for faster access */
	private final int numChannels;
	

	/**
	 * Basic constructor, meant to write an image to disk. It needs
	 * to know right away the basic layout of the image, namely its
	 * width, height and channel configuration. This constructor
	 * <em>does not</em> allocate the pixels buffer.
	 * 
	 * @param width the desired width of the image, greater than zero.
	 * @param height the desired height of the image, greater than zero.
	 * @param channels the channels configuration of the image, non null.
	 * @see #allocateBuffer()
	 */
	public EXRSimpleImage(int width, int height, Channels channels){
		
		if (width <= 0 || height <= 0) {
			throw new IllegalArgumentException("The image's dimensions are " +
					"invalid (less than or equal to 0).");
		}
		
		this.width    = width;
		this.height   = height;
		this.channels = channels;
		this.numChannels = channels.getNumChannels();
	}

	/**
	 * Creates a new instance of the image by reading the OpenEXR file at
	 * <code>filename</code>, which is read as a simple RGB/RGBA file.
	 * 
	 * @param file the path of the image to be read.
	 * @param channels the desired channel configuration for the file.
	 * @throws EXRIOException if an error occurs while reading.
	 */
	public EXRSimpleImage(File file, Channels channels) throws EXRIOException {
		
		if (file == null) {
			throw new IllegalArgumentException("Cannot take null paths.");
		}
		if (!file.canRead()) {
			throw new EXRIOException("Cannot read the file \"" + file + "\"");
		}
		
		OpenEXRTo to = new OpenEXRTo();
		read(to, channels.getNumChannels(), file.getAbsolutePath());

		this.buffer      = to.buffer;
		this.height      = to.height;
		this.width       = to.width;
		this.attrib      = to.attrib;
		this.channels    = channels;
		this.numChannels = channels.getNumChannels();
	}
	
	/**
	 * Sugar constructor.
	 * 
	 * @param filename path to file to read from.
	 * @param channels the desired channel configuration for the file.
	 * @throws EXRIOException if an error occurs while reading.
	 * @see #EXRSimpleImage(File, Channels)
	 */
	public EXRSimpleImage(String filename, Channels channels) 
		throws EXRIOException {
		this(new File(filename), channels);
	}
	
	/**
	 * <p>Sets the buffer containing the pixels of the image. The buffer
	 * contains <code>width*height</code> interleaved pixels stored in scanline order, where
	 * (0,0) is the top-left corner and (width-1,height-1) is the bottom-right
	 * corner. Therefore the size of the buffer must be at least
	 * <code>width*height*(number of channels)</code>.</p>
	 * 
	 * <p>This method just does a shallow copy (ie only copies the reference)
	 * to the input buffer for efficiency.</p>
	 * 
	 * @param buffer a non null array holding the interleaved pixels of the image.
	 */
	public void setBuffer(float[] buffer) {
		if (buffer == null) {
			throw new IllegalArgumentException("The buffer cannot be null.");
		}
		else if(buffer.length < width*height*numChannels) {
			throw new IllegalArgumentException(
					"The buffer does not have enought elements.");
		}
		
		this.buffer = buffer;
	}
	
	/**
	 * Sets the pixel buffer of this image to a new buffer. The previous
	 * buffer is simply discarded.
	 */
	public void allocateBuffer() {
		buffer = new float[width*height*numChannels];
	}
	
	/**
	 * Returns a reference to the pixel buffer. This method provides
	 * total control upon the data, so it must be handled with care!
	 * 
	 * @return the pixel buffer.
	 */
	public float[] getBuffer() {
		return buffer;
	}
	
	/**
	 * Returns the channel configuration of the image.
	 * 
	 * @return the channels.
	 */
	public Channels getChannels() {
		return channels;
	}
	
	/**
	 * Sugar method, it is equivalent to 
	 * <code>getChannels.getNumChannels()</code>.
	 * 
	 * @return the number of data channels per pixel.
	 * @see #getChannels()
	 */
	public int getNumChannels() {
		return numChannels;
	}
	
	/**
	 * Returns a reference to the attributes of the image,
	 * or <code>null</code> if the image doesn't contain any.
	 * This reference may be used to edit the attributes of the image.
	 * 
	 * @return the attributes or <code>null</code>.
	 */
	public Attributes getAttributes() {
		return attrib;
	}
	
	/**
	 * Sets the additional attributes of this image or <code>null</code>.
	 * 
	 * @param attrib the new set of attributes.
	 */
	public void setAttributes(Attributes attrib) {
		this.attrib = attrib;
	}
	
	/**
	 * Returns the width of the image.
	 * 
	 * @return the width.
	 */
	public int getWidth() {
		return width;
	}
	
	/**
	 * Returns the height of the image.
	 * 
	 * @return the height.
	 */
	public int getHeight() {
		return height;
	}
	
	/**
	 * Returns the index in the data buffer where the first element
	 * of the pixel at (x,y) is found. This method performs bounds checking.
	 * 
	 * @param x width coordinate in the range  [0,width-1];
	 * @param y height coordinate in the range [0,height-1];
	 * @return the index in the buffer of the (x,y) pixel.
	 */
	public int getIndex(int x, int y) {
		if (x < 0 || x >= width) {
			throw new IndexOutOfBoundsException("x is out of bounds:" + x);
		}
		if (y < 0 || y >= height) {
			throw new IndexOutOfBoundsException("y is out of bounds:" + y);
		}
		return numChannels*(y*width + x);
	}
	
	/**
	 * The same as {@link #getIndex(int, int)} but without 
	 * bounds checking, use at your own risk.
	 * 
	 * @param x width coordinate in the range  [0,width-1];
	 * @param y height coordinate in the range [0,height-1];
	 * @return the index in the buffer of the (x,y) pixel.
	 */
	public int getIndexFast(int x, int y) {
		return numChannels*(y*width + x);
	}
	
	/**
	 * Returns a newly allocated array with the data of
	 * the pixel at (x,y). This is a convenience method
	 * with state checking, thus it is not very fast.
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @return the pixel (x,y).
	 */
	public float[] getPixel(int x, int y) {
		if (buffer == null) {
			throw new IllegalStateException("The pixel buffer is null.");
		}
		final int index = getIndex(x,y);
		float[] pixel = new float[numChannels];
		System.arraycopy(buffer, index, pixel, 0, pixel.length);
		return pixel;
	}
	
	/**
	 * Copies the (x,y) pixel elements starting at the location
	 * <code>destPos</code> (0-based) in the <code>destBuffer</code>. So it
	 * will use the destination buffer at the array indices
	 * <code>[destPost, destPos+getNumChannels()-1]</code>.
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @param destBuffer array where the pixel values will be written.
	 * @param destPos starting index in the array for the copied pixel data.
	 */
	public void getPixel(int x, int y, float[] destBuffer, int destPos) {
		if (buffer == null) {
			throw new IllegalStateException("The pixel buffer is null.");
		}
		for(int i = 0, index = getIndex(x,y); 
			i < numChannels; ++i) {
			
			destBuffer[destPos++] = buffer[index++];
		}
		
	}
	
	/**
	 * Sugar method, it is equivalent to
	 * <code>getPixel(x,y,destPixel,0)</code>. This is, the pixel
	 * data is copied to the beginning of <code>destPixel</code>.
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @param destPixel array where the pixel values will be written.
	 * @see #getPixel(int, int, float[], int)
	 */
	public void getPixel(int x, int y, float[] destPixel) {
		getPixel(x, y, destPixel, 0);
	}
	
	/**
	 * Retrieves a single element from the (x,y) pixel. The
	 * element index <code>channelIndex</code> is in the range
	 * <code>[0, getNumChannels()-1]</code>
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @param channelIndex index of the pixel element to return.
	 * @return the chanellIndex-th element of pixel (x,y).
	 */
	public float getPixelElement(int x, int y, int channelIndex) {
		if (buffer == null) {
			throw new IllegalStateException("The pixel buffer is null.");
		}
		if (channelIndex < 0 || channelIndex >= numChannels) {
			throw new IndexOutOfBoundsException(
					"Channel index out of bounds: " + channelIndex);
		}
		final int index = getIndex(x,y);
		return buffer[index+channelIndex];
	}
	
	/**
	 * Sets the value of the (x,y) pixel's channels. The <i>n</i>-th value
	 * in <code>elements</code> sets the <i>n</i>-th value of the pixel.
	 * All the pixel elements must be set, but if there are more elements
	 * than pixel channels, the extra data is simply ignored. Hence
	 * the length of <code>elements</code> must be at least
	 * the same {@link #getNumChannels()}.
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @param elements the new values for pixel (x,y).
	 */
	public void setPixel(int x, int y, float... elements) {
		if (buffer == null) {
			throw new IllegalStateException("The pixel buffer is null.");
		}
		if (elements.length < numChannels) {
			throw new IllegalArgumentException(
					"Insufficient number of elements: " + elements.length);
		}
		for (int i = 0, index = getIndex(x,y); i < numChannels; ++i) {
			buffer[index++] = elements[i];
		}
		
	}
	
	/**
	 * Sets the value of the <i>channelIndex</i>-th component
	 * (0 based indexing) of the (x,y) pixel.
	 * 
	 * @param x x-coordinate of the pixel.
	 * @param y y-coordinate of the pixel.
	 * @param channelIndex which pixel channel to choose.
	 * @param val the new value to set.
	 */
	public void setPixelElement(int x, int y, int channelIndex, float val) {
		if (buffer == null) {
			throw new IllegalStateException("The pixel buffer is null.");
		}
		if (channelIndex < 0 || channelIndex >= numChannels) {
			throw new IndexOutOfBoundsException(
					"Channel index out of bounds: " + channelIndex);
		}
		final int index = getIndex(x, y);
		buffer[index+channelIndex] = val;
	}
	
	
	/**
	 * Writes the current image to disk as an OpenEXR file using
	 * the specified <code>file</code>, which should have the suffix
	 * <code>.exr</code>. Before calling this method the buffer must
	 * have been set to meaningful values.
	 * 
	 * @param file path where the file will be saved.
	 * @param compression the type of compression used when creating the file.
	 * @throws EXRIOException if an error occur during writing.
	 */
	public void write(File file, Compression compression) throws EXRIOException {
		String filename = file.getAbsolutePath();
			
		if (filename == null || filename.length() == 0) {
			throw new IllegalArgumentException("Invalid filename: " + filename);
		}
		if (compression == null) {
			throw new IllegalArgumentException(
					"The compression type cannot be null.");
		}
		
		if (buffer == null || buffer.length < (width*height*numChannels)) {
			throw new IllegalStateException("The buffer is inconsistent.");
		}
		if (width <= 0 || height <= 0) {
			throw new IllegalStateException("Wrong dimensions!");
		}
		
		// To to C++ to get this done
		write(filename, this.buffer, this.width, this.height,
				numChannels, this.attrib, compression.getFlag());
		
	}
	
	/**
	 * Utility method, it is the same as calling
	 * {@link #write(File, Compression)}, constructing
	 * the file with the given filename.
	 * 
	 * @param filename path where the file will be saved.
	 * @param compression the type of compression used when creating the file.
	 * @throws EXRIOException if an error occur during writing.
	 */
	public void write(String filename, Compression compression) 
		throws EXRIOException {
		
		write(new File(filename), compression);
	}
	
	

	
	/*
	 * Calls the native method for saving the file.
	 * Assumes that neither name nor pixels is null, width > 0, height > 0, and that pixels
	 * has already been initialized as a direct buffer of size (width * height * 3 * 4) for
	 * holding the float RGB pixels. It also assumes that compression is one of the native
	 * flags specified in the compression enumeration
	 */
	private static native void write(String name, float[] pixelBuffer, 
			int width, int height, 
			int numChannels, Attributes attrib, int compressionFlag) 
			throws EXRIOException;
	
	/*
	 * Initializes a freshly created instance of OpenEXRTo (transfer object) with the
	 * contents from the file, setting its dataBuffer with floatRGB pixels
	 * as well as setting the appropriate width and height values. It case
	 * of error it will throw an exception.
	 * It also receives the number of channels that will be saved into
	 * the transfer object: 3 for RGB or 4 for full RGBA.
	 */
	private native static void read(OpenEXRTo to, int numChannels,
			String filename) throws EXRIOException;
	
	/*
	 * Returns the serialVersionID of the class as stored in the native library.
	 */
	private native static long getNativeVersion();
	
	/*
	 * Sets the number of threads for reading/writing files. It only
	 * makes sense to call this with 2 or more threads.
	 */
	private native static void setNumWorkingThreads(int numThreads);
	
	/*
	 * Initializes the native method,field and classes IDs 
	 */
	private native static void initCache();

}

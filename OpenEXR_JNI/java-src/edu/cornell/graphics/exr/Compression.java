package edu.cornell.graphics.exr;

/**
 * Enumeration which represents the different compression methods
 * available for saving files of OpenEXR 1.6
 * 
 * @author edgar
 */
public enum Compression {
	
	/**
	 * No compression is applied at all.
	 */
	NONE(0),
	
	/**
	 * Differences between horizontally adjacent pixels are run-length encoded. 
	 * This method is fast, and works well for images with large flat areas, 
	 * but for photographic images, the compressed file size is usually 
	 * between 60 and 75 percent of the uncompressed size.
	 */
	RLE(1),
	
	/**
	 * The same as <code>ZIP</code>, but just one scan line at a time.
	 * 
	 * @see #ZIP
	 */
	ZIPS(2),
	
	/**
	 * <p>Differences between horizontally adjacent pixels are compressed using the
	 * open source zlib library. ZIP decompression is faster than PIZ decompression,
	 * but ZIP compression is significantly slower. Photographic images tend to 
	 * shrink to between 45 and 55 percent of their uncompressed size.</p> 
	 * 
	 * <p>Multiresolution files are often used as texture maps for 3D renderers. 
	 * For this application, fast read accesses are usually more important than 
	 * fast writes, or maximum compression. 
	 * For texture maps, ZIP is probably the best compression method.</p>
	 * 
	 * <p>This method encodes in blocks of 16 scan lines.</p>
	 */
	ZIP(3),
	
	/**
	 * A wavelet transform is applied to the pixel data, and the result is 
	 * Huffman-encoded. This scheme tends to provide the best compression ratio 
	 * for the types of images that are typically processed at 
	 * Industrial Light & Magic. Files are compressed and decompressed at 
	 * roughly the same speed. For photographic images with film grain, 
	 * the files are reduced to between 35 and 55 percent of their uncompressed size.
	 * PIZ compression works well for scan-line-based files, and also for tiled files 
	 * with large tiles, but small tiles do not shrink much. 
	 * (PIZ-compressed data start with a relatively long header; if the input to 
	 * the compressor is short, adding the header tends to offset any size 
	 * reduction of the input.)
	 */
	PIZ(4),
	
	/**
	 * After reducing 32-bit floating-point data to 24 bits by rounding, 
	 * differences between horizontally adjacent pixels are compressed with 
	 * zlib, similar to ZIP. PXR24 compression preserves image channels of t
	 * ype HALF and UINT exactly, but the relative error of FLOAT data increases 
	 * to about 3×10<sup>-5</sup>. This compression method works well for depth buffers 
	 * and similar images, where the possible range of values is very large, 
	 * but where full 32-bit floating-point accuracy is not necessary. 
	 * Rounding improves compression significantly by eliminating the pixels' 8 
	 * least significant bits, which tend to be very noisy, and difficult to compress.
	 */
	PXR24(5, false),
	
	/**
	 * Channels of type HALF are split into blocks of four by four pixels or 32 
	 * bytes. Each block is then packed into 14 bytes, reducing the data to 44 
	 * percent of their uncompressed size. When B44 compression is applied to 
	 * RGB images in combination with luminance/chroma encoding (see below), 
	 * the size of the compressed pixels is about 22 percent of the size of the 
	 * original RGB data. Channels of type UINT or FLOAT are not compressed.
	 */
	B44(6, false),
	
	/**
	 * The same as <code>B44</code>, but the flat fields are compressed more.
	 * 
	 * @see #B44
	 */
	B44A(7, false);


	/** The number used in the native enumeration to represent this
	 * compression method, as defined in the enum <code>Imf::Compression</code>,
     * defined in <code>ImfCompression.h</code>.*/
	final private int flagVal;
	
	/** To know whether the method is lossless or lossy */
	final private boolean lossless;
	
	// The full specific constructor
	Compression(int flag, boolean lossless) {
		this.flagVal = flag;
		this.lossless = lossless;
	}
	
	// Easy constructor for lossless methods
	Compression(int flag) {
		this(flag, true);
	}
	
	/**
	 * Returns the value which corresponds to the native ImfCompression
	 * enumeration representing this compression method.
	 * 
	 * @return an integer with the native enumeration value.
	 */
	protected int getFlag() { return flagVal; }
	
	/**
	 * Indicates the kind of compression method.
	 * Compressing an image with a lossless method preserves 
	 * the image exactly; the pixel data are not altered. 
	 * Compressing an image with a lossy method preserves the image 
	 * only approximately; the compressed image looks like the original, 
	 * but the data in the pixels may have changed slightly.
	 * 
	 * @return <code>true</code> if the method is lossless, <code>false</code>
	 *         if it is lossy.
	 */
	public boolean isLossless() { return lossless; }
}

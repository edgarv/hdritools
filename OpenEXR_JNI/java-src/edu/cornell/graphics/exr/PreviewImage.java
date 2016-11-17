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

import java.util.Arrays;

/**
 * A usually small, low-dynamic range image, that is intended to be stored
 * in an image file's header.
 * 
 * <p>The pixel data consists of interleaved RGBA8 pixels in scan line order.
 * For the red, green and blue pixel components, intensity is proportional to
 * {@code pow(x/255, 2.2)} where {@code x} is {@code r}, {@code g} or {@code b}.
 * For alpha {@literal 0} is transparent and {@literal 255} fully opaque.</p>
 * 
 * <p>The {@code x} and {@code y} coordinates of the pixels data array
 * go from {@literal 0} to {@code w-1}, and from
 * {@literal 0} to {@code h-1} respectively; {@code w} corresponds to
 * {@link #getWidth() } and {@code h} to {@link #getHeight() }. The pixel with
 * coordinates {@code (x,y)} is at array index {@code 4*((y*w)+x)}.
 * Pixel {@code (0,0)} is the upper left corner of the preview image.</p>
 * 
 * @since OpenEXR-JNI 2.1
 */
public class PreviewImage implements Cloneable {
    private final int width;
    private final int height;
    private final byte[] pixelData;
    
    /**
     * Constructs a new preview image with {@code width} by {@code height}
     * pixels initialized with {@code (r=0, b=0, g=0, a=255)}.
     * 
     * @param width  width of the preview image
     * @param height height of the preview image
     * @throws IllegalArgumentException if either {@code width} or
     *         {@code height} is negative.
     */
    public PreviewImage(int width, int height) {
        this(width, height, null, 0, false);
    }
    
    /**
     * Constructs a new preview image with {@code width} by {@code height}
     * pixels initialized with the values from {@code pixels}.
     * 
     * @param width  width of the preview image
     * @param height height of the preview image
     * @param pixels initial values for the preview image's pixels
     * @throws NullPointerException if {@code pixels} is null
     * @throws IllegalArgumentException if either {@code width} or
     *         {@code height} is negative, or there is not enough data
     *         in {@code pixels}.
     */
    public PreviewImage(int width, int height, byte[] pixels) {
        this(width, height, pixels, 0, true);
    }
    
    /**
     * Constructs a new preview image with {@code width} by {@code height}
     * pixels initialized with the values from {@code pixels} starting at
     * {@code offset}.
     * 
     * @param width  width of the preview image
     * @param height height of the preview image
     * @param pixels initial values for the preview image's pixels
     * @param offset starting position in {@code pixels}
     * @throws NullPointerException if {@code pixels} is null
     * @throws IllegalArgumentException if either {@code width} or
     *         {@code height} is negative, or there is not enough data
     *         in {@code pixels}.
     */
    public PreviewImage(int width, int height, byte[] pixels, int offset) {
        this(width, height, pixels, offset, true);
    }
    
    /**
     * Constructs a new preview image with {@code width} by {@code height}
     * pixels initialized with the values from {@code pixels} starting at
     * {@code offset}. If {@code pixels} is {@code null} initializes the pixels
     * with {@code (r=0, b=0, g=0, a=255)}.
     * 
     * @param width  width of the preview image
     * @param height height of the preview image
     * @param pixels initial values for the image's pixels or {@code null}
     * @param offset starting position in {@code pixels}
     * @param checkNull if {@code true}, will throw NullPointerException
     *         if pixels is null.
     * @throws IllegalArgumentException if either {@code width} or
     *         {@code height} is negative, or there is not enough data
     *         in {@code pixels}.
     */
    private PreviewImage(int w, int h, byte[] p, int off, boolean checkNull) {
        if (w < 0) {
            throw new IllegalArgumentException("Invalid width: " + w);
        }
        if (h < 0) {
            throw new IllegalArgumentException("Invalid height: " + h);
        }
        if (checkNull && p == null) {
            throw new NullPointerException("null reference pixel data");
        }
        this.width  = w;
        this.height = h;
        final int numBytes = 4 * (w * h);
        this.pixelData = new byte[numBytes];
        if (p != null) {
            if ((p.length - off) < numBytes) {
                throw new IllegalArgumentException("Insufficient pixel data");                
            }
            System.arraycopy(p, off, this.pixelData, 0, numBytes);
        } else {
            // Per Java's rules, the pixel data contains all zeros at this point
            // so we just set the alpha
            for (int i = 3; i < numBytes; i += 4) {
                pixelData[i] = (byte) 255;
            }
        }
    }
    
    /**
     * Returns the width of this preview image.
     * @return the width of this preview image
     */
    public int getWidth() {
        return width;
    }
    
    /**
     * Returns the height of this preview image.
     * @return the height of this preview image
     */
    public int getHeight() {
        return height;
    }
    
    /**
     * Returns a reference to the pixel data array of this preview image.
     * @return a reference to the pixel data array of this preview image
     */
    public byte[] getPixelData() {
        return pixelData;
    }
    
    /**
     * Returns the index in the pixel data array for the pixel {@code (x,y)}.
     * The index is computed as {@code 4*((y*w)+x)} (each pixel has 4 bytes.)
     * 
     * @param x the pixel's {@code x} coordinate
     * @param y the pixel's {@code y} coordinate
     * @return the index in the pixel data array for the pixel {@code (x,y)}
     * @throws IndexOutOfBoundsException if either parameter is out of range
     */
    public int index(int x, int y) {
        if (x < 0 || x >= width) {
            throw new IndexOutOfBoundsException("invalid x: " + x);
        }
        if (y < 0 || y >= height) {
            throw new IndexOutOfBoundsException("invalid y: " + y);
        }
        return 4 * (y*width + x);
    }

    /**
     * Returns a hash code for this preview image. The hash code for a 
     * {@code PreviewImage} object is computed with its dimensions and the
     * contents of its pixel data array.
     * 
     * @return a hash code for this preview image
     */
    @Override
    public int hashCode() {
        int hash = 7;
        hash = 67 * hash + this.width;
        hash = 67 * hash + this.height;
        hash = 67 * hash + Arrays.hashCode(this.pixelData);
        return hash;
    }

    /**
     * Compares this preview image to the specified object.  The result is
     * {@code true} if and only if the argument is not {@code null}, is a
     * {@code PreviewImage} of the same size as this instance, and their pixels
     * values are all equal.
     *
     * @param obj The object to compare this {@code PreviewImage} against
     * @return {@code true} if the given object represents a
     *         {@code PreviewImage} equivalent to this instance,
     *         {@code false} otherwise
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final PreviewImage other = (PreviewImage) obj;
        if (this.width != other.width) {
            return false;
        }
        if (this.height != other.height) {
            return false;
        }
        if (!Arrays.equals(this.pixelData, other.pixelData)) {
            return false;
        }
        return true;
    }

    /**
     * Returns a string representation of this preview image.
     * 
     * <p>The {@code toString} method for class {@code PreviewImage}
     * returns a string consisting of the prefix <tt>"PreviewImage{"</tt>
     * and its dimensions:
     * <blockquote>
     * <tt>"PreviewImage{" + "width=" + width + ", height=" + height + '}'
     * </tt></blockquote>
     * 
     * @return a string representation of this preview image
     */
    @Override
    public String toString() {
        return "PreviewImage{" + "width=" + width + ", height=" + height + '}';
    }

    /**
     * Creates and returns a copy of this preview image. The cloned preview
     * image has the same dimensions and pixel contents as the calling instance,
     * with its own, independent pixel data array.
     * 
     * @return a clone of this preview image
     */
    @Override
    public PreviewImage clone() {
        PreviewImage p = new PreviewImage(width, height, pixelData);
        return p;
    }
    
}

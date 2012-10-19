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

import java.io.Closeable;
import java.io.File;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

/**
 * More general interface to read OpenEXR files. The main purpose of this
 * class is to read files which may contain an arbitrary number of channels.
 * This class is not as general as the OpenEXR classes in the reference
 * implementation from ILM. Its main limitations are:
 * <ul>
 * <li>Only channels with <code>(1,1)</code> sampling are supported.</li>
 * <li>It can only read images from files.</li>
 * <li>Reads only attributes of the types:
 *  <ul>
 *      <li>String</li>
 *      <li>String vector</li>
 *      <li>Integer</li>
 *      <li>Float</li>
 *      <li>Double</li>
 *      <li>{@linkplain Compression}</li>
 *  </ul>
 * </li>
 * <li>The class is not thread-safe</li>
 * <li>The EXR data window is ignored, the class only stores width and height.</li>
 * <li>The class does not support multiple images. All channels have the
 * same resolution.</li>
 * </ul>
 * 
 * @author edgar
 */
public class BasicInputFile implements Closeable {
    
    /** Maximum admissible length for a channel name as of OpenEXr 1.7.1 */
    private final static int MAX_CHANNEL_LENGTH = 255;
    
    /** Set of channels present in the file */
    private LinkedHashSet<String> channelNames = new LinkedHashSet<String>();
    
    /** Map of supported attributes */
    private HashMap<String, Object> attributes = new HashMap<String, Object>();
    
    /** Handle to the native class doing the actual IO */
    private long handle = 0L;
    
    /** Width of the image */
    private int width = 0;
    
    /** Height of the image */
    private int height = 0;
    
    
    
    BasicInputFile(File file) throws EXRIOException {
        this(file.getAbsolutePath());
    }
    
    BasicInputFile(String filename) throws EXRIOException {
        if (filename == null) {
            throw new IllegalArgumentException("Null filename");
        } else if (filename.isEmpty()) {
            throw new IllegalArgumentException("Empty filename");
        }
        
        // Initialize the member variables with the native method
        initFile(filename);
        
        if (handle == 0L) {
            throw new IllegalStateException("Null handle");
        }
        if (width < 1 || height < 1) {
            throw new IllegalStateException(String.format("Invalid image " +
                    "size: [%4d x %4d]", width, height));
        }
        
        // Modify compression into the appropriate java type
        Compression compression = getJavaCompression();
        attributes.put("compression", compression);
    }
    
    private Compression getJavaCompression() {
        Object obj = attributes.get("compression");
        if (obj == null) {
            throw new IllegalStateException("Missing compression attribue.");
        } else if (!(obj instanceof Integer)) {
            throw new IllegalStateException("Unexpected compression: " + obj);
        }
        final int c = (Integer)obj;
        switch(c) {
            case 0:
                return Compression.NONE;
            case 1:
                return Compression.RLE;
            case 2:
                return Compression.ZIPS;
            case 3:
                return Compression.ZIP;
            case 4:
                return Compression.PIZ;
            case 5:
                return Compression.PXR24;
            case 6:
                return Compression.B44;
            case 7:
                return Compression.B44A;
            default:
                throw new IllegalStateException("Unknown compression: "+ c);
        }
    }
    
    
    public Map<String, float[]> read(String... channels) throws EXRIOException,
            NoSuchChannelException {
        return read(new HashSet<String>(Arrays.asList(channels)));
    }
    
    public Map<String, float[]> read(Collection<? extends String> channels)
            throws EXRIOException, NoSuchChannelException {
        return read(new HashSet<String>(channels));
    }
    
    public Map<String, float[]> readAll() throws EXRIOException {
        return read(channelNames);
    }
    
    public Map<String, float[]> read(Set<? extends String> channels)
            throws EXRIOException, NoSuchChannelException {
        if (channels == null) {
            throw new IllegalArgumentException("Null channel set");
        } else if (channels.isEmpty()) {
            throw new IllegalArgumentException("Empty channel set");
        }
        
        // Verify the state
        if (handle == 0) {
            throw new IllegalStateException("Null handle");
        }
        
        // Verify that channel names are valid
        if (channels != channelNames) {
            for (String c : channels) {
                if (c == null) {
                    throw new IllegalArgumentException("Null channel name");
                } else if (c.isEmpty()) {
                    throw new IllegalArgumentException("Empty channel name");
                } else if (c.length() > MAX_CHANNEL_LENGTH) {
                    throw new IllegalArgumentException("Channel name too long: "
                            + c + "(" + c.length() + ")");
                } else if (!channelNames.contains(c)) {
                    throw new NoSuchChannelException(c);
                }
            }
        }
        
        // Allocate the result
        HashMap<String, float[]> result = new HashMap<String, float[]>();
        final int len = getWidth() * getHeight();
        for (String c : channels) {
            result.put(c, new float[len]);
        }
        
        // Call the native method
        readPixels(handle, result);
        return result;
    }
    
    
    /**
     * Returns an unmodifiable set with the channels present in the file.
     * @return an unmodifiable set with the channels present in the file.
     */
    public Set<String> getChannelNames() {
        return java.util.Collections.unmodifiableSet(channelNames);
    }
    
    
    /**
     * Returns the attribute associated with the given name, or
     * <code>null</code> if the file does not contain the given attribute.
     * 
     * @param name the name of the desired attribute.
     * @return the value of the attribute or <code>null</code> if there is
     * no such attribute in the file.
     */
    public Object getAttribute(String name) {
        if (name == null) {
            throw new IllegalArgumentException("Null attribute name");
        } else if (name.isEmpty()) {
            throw new IllegalArgumentException("Empty name");
        }
        return attributes.get(name);
    }
    
    
    /**
     * Return the width of the image in pixels.
     * @return the width of the image in pixels.
     */
    public int getWidth() {
        assert(width > 0);
        return width;
    }
    
    /**
     * Return the height of the image in pixels.
     * @return the height of the image in pixels.
     */
    public int getHeight() {
        return height;
    }

    @Override
    public void close() throws EXRIOException {
        if (handle != 0) {
            throw new UnsupportedOperationException("Not supported yet.");
        }
    }

    
    
    private static native void initFile(String filename);
    
    private static native void readPixels(long handle,
            HashMap<String, float[]> result);
}

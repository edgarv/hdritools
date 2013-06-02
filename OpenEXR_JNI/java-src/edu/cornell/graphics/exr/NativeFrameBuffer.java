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
import java.nio.ByteBuffer;
import java.util.Map;

/**
 * Package-only class to generate a native {@code Imf::FrameBuffer} instance.
 * The intended workflow is as follows:
 * <ol>
 *   <li>Create a new instance of {@code NativeFrameBuffer}. This will also
 *   create the native instance.</li>
 *   <li>Access the native instance through {@code getNativeHandle()}</li>
 *   <li>Once the native instance is no longer needed, delete it via
 *   {@code deleteNativeHandle}. To avoid memory leaks this should be called
 *   as soon as possible.</li>
 *   <li>Even after the native handle is destroyed, it is still possible to
 *   access the Java-sided frame buffer data.</li>
 * </ol>
 * 
 * @since 0.3
 */
class NativeFrameBuffer {
    
    static {
        // TODO Unify native library loading
        System.loadLibrary("openexrjni");
        initNativeCache();
    }
    
    /**
     * Creates a new instance by deep-copying the input {@code frameBuffer}
     * and allocating a new native instance of that copy.
     * 
     * <p>The source frame buffer is checked for basic consistency against
     * the given channel list before duplicating it. If the source frame buffer
     * contains error such as missing slices or negative sampling factors this
     * constructors throws {@code IllegalArgumentException}. If <em>any</em>
     * slice does <em>not</em> use direct byte buffers it throws
     * {@code UnsupportedOperationException}.</p>
     * 
     * @param frameBuffer original non-null frame buffer to copy
     * @param channels non-null channel list to validate against
     * @throws IllegalArgumentException if the source frame buffer does not
     *         meet the preconditions
     * @throws UnsupportedOperationException if any slice does not use a
     *         direct byte buffer
     * @see ByteBuffer#isDirect() 
     */
    NativeFrameBuffer(FrameBuffer frameBuffer, ChannelList channels) throws
            IllegalArgumentException, UnsupportedOperationException  {
        fb = validatedFrameBufferCopy(frameBuffer, channels);
        fbHash = fb.hashCode();
        
        // Once all slices survived the baseline checks, update the native part
        String[] names = new String[fb.size()];
        Slice[] slices = new Slice [fb.size()];
        int count = 0;
        for (Map.Entry<String, Slice> fbEntry : fb) {
            names[count]  = fbEntry.getKey();
            slices[count] = fbEntry.getValue();
            ++count;
        }
        nativeHandle = newNativeFrameBuffer(count, names, slices);
        if (nativeHandle == 0L) {
            throw new IllegalStateException("invalid native handle");
        }
    }
    
    /**
     * Returns a handle to the native {@code Imf::FrameBuffer} instance.
     * @return a handle to the native {@code Imf::FrameBuffer} instance
     * @throws IllegalStateException if the native instance has already been
     *         destroyed via {@code deleteNativeHandle()}.
     */
    long getNativeHandle() {
        if (nativeHandle != 0L) {
            return nativeHandle;
        } else {
            throw new IllegalStateException("native handle already destroyed");
        }
    }
    
    /**
     * Deletes the underlying native {@code Imf::FrameBuffer} instance.
     * @throws IllegalStateException if the native instance has already been
     *         destroyed via {@code deleteNativeHandle()}.
     */
    void deleteNativeHandle() {
        if (nativeHandle != 0L) {
            deleteNativeHandle(nativeHandle);
            nativeHandle = 0L;
        } else {
            throw new IllegalStateException("native handle already destroyed");
        }
    }
    
    /**
     * Returns a reference to the Java frame buffer.
     * 
     * <p>For efficiency this function returns the actual internal frame buffer 
     * reference, thus callers ought <em>not</em> to modify the returned
     * instance. In contrast to C++, returning a completely read-only frame
     * buffer would require a lot of changes, including supporting read-only
     * slices with only-final accessible fields and/or read-only aware setters.
     * As a safety feature the implementation caches the hash code of the 
     * frame buffer after calling {@code setFrameBuffer} and compares it
     * against the current frame buffer's hash code. It they
     * do not match it throws an {@code IllegalStateException}.</p>
     * 
     * <p>Because of the validation, clients which will use the frame buffer for
     * multiple operations should call {@code getFrameBuffer} only once and then
     * use the validated reference.</p>
     * 
     * @return a reference to the Java frame buffer
     * @throws IllegalStateException if the frame buffer has been modified
     *         externally
     */
    FrameBuffer getFrameBuffer() {
        assert fb != null;
        if (fbHash == fb.hashCode()) {
            return fb;
        } else {
            throw new IllegalStateException("frame buffer has been modified");
        }
    }
    
    /**
     * Verifies that native I/O operations on this frame buffer will access,
     * for each slice, the valid range between its buffer's {@code position()}
     * and {@code limit()}. The range is validated for the given non-null
     * data window, as defined in the OpenEXR format, for the contiguous range
     * of scan lines
     * {@code [min(scanLine1,scanLine2), max(scanLine1,scanLine2)]}.
     * 
     * <p>The purpose of this method is to avoid hard to debug illegal access
     * exception errors. The way the OpenEXR library specifies offsets for the
     * slices is not very intuitive: sometimes it might require negative offsets
     * which might seem wrong. If the conditions do not hold the method throws 
     * {@code IndexOutBoundsException}. If the frame buffer has been externally
     * modified it throws {@code IllegalStateException}.</p>
     * 
     * @param dw the data window used to validate this frame buffer
     * @param scanLine1 the first scan line in the range
     * @param scanLine2 the second scan line in the range
     * @throws IllegalStateException if the frame buffer has been modified
     * @throws IndexOutOfBoundsException if an operation would fall outside a
     *         slice buffer's {@code position()} and {@code limit()}
     * @see ByteBuffer#position() 
     * @see ByteBuffer#limit() 
     */
    void validateForDataWindow(final Box2<Integer> dw,
            final int scanLine1, final int scanLine2) throws
            IllegalStateException, IndexOutOfBoundsException {
        assert fb != null;
        if (fbHash != fb.hashCode()) {
            throw new IllegalStateException("frame buffer has been modified");
        }
        final int scanLineMin = Math.min(scanLine1, scanLine2);
        final int scanLineMax = Math.max(scanLine1, scanLine2);
        if (scanLineMin < dw.yMin || scanLineMax > dw.yMax) {
            throw new IndexOutOfBoundsException("invalid scanline range: (" +
                    scanLineMin + ',' + scanLineMax + ')');
        }
        
        for (Map.Entry<String, Slice> fbEntry : fb) {
            Slice slice = fbEntry.getValue();
            assert slice.xSampling >= 1 && slice.ySampling >= 1;
            double xFactor = (1.0/slice.xSampling) * slice.xStride;
            double yFactor = (1.0/slice.ySampling) * slice.yStride;
            int minPosition = (int)Math.floor(slice.baseOffset +
                    Math.min(xFactor*dw.xMin, xFactor*dw.xMax) +
                    Math.min(yFactor*scanLineMin, yFactor*scanLineMax));
            int maxPosition = (int)Math.ceil(slice.baseOffset +
                    Math.max(xFactor*dw.xMin, xFactor*dw.xMax) +
                    Math.max(yFactor*scanLineMin, yFactor*scanLineMax)) +
                    slice.type.byteSize();
            
            if (minPosition < slice.buffer.position()) {
                throw new IndexOutOfBoundsException("reading data into the " +
                        "slice for channel \"" + fbEntry.getKey() + "\" " +
                        "would be behind its buffer position");
            }
            if (maxPosition > slice.buffer.limit()) {
                throw new IndexOutOfBoundsException("reading data into the " +
                        "slice for channel \"" + fbEntry.getKey() + "\" " +
                        "would exceed its buffer limit");
            }
        }
    }
    
    private static FrameBuffer validatedFrameBufferCopy(
            final FrameBuffer fbSource, final ChannelList channels) throws
            IllegalArgumentException, UnsupportedOperationException {
        assert fbSource != null && channels != null;
        final FrameBuffer fbValidatedCopy = new FrameBuffer();
        for (Map.Entry<String, Slice> fbEntry : fbSource) {
            String name = fbEntry.getKey();
            Slice slice = fbEntry.getValue();
            
            // Check the slice for invalid values
            if (slice.type == null) {
                throw new IllegalArgumentException("there is no pixel type " +
                        "for slice \"" + name + "\"");
            } else if (slice.type == PixelType.UINT) {
                if (Double.isInfinite(slice.fillValue) || 
                    Double.isNaN(slice.fillValue) ||
                    slice.fillValue < -0xffffffffL ||
                    slice.fillValue > 0xffffffffL) {
                    throw new IllegalArgumentException("slice \""+name+"\": " +
                            "invalid fill value: " + slice.fillValue);                   
                }
            }
            
            if (slice.buffer == null) {
                throw new IllegalArgumentException("there is no frame buffer " +
                        "for slice \"" + name + "\"");
            } else if (!slice.buffer.isDirect()) {
                throw new UnsupportedOperationException("slice \""+name+"\": " +
                        "non-direct buffers support is not implemented yet");
            }
            
            if (slice.xSampling < 1 || slice.ySampling < 1) {
                throw new IllegalArgumentException("slice \""+name+"\": " +
                        "invalid stride (" + slice.xSampling + ',' + 
                        slice.ySampling + ')' );
            }
            Channel channel = channels.findChannel(name);
            if (channel != null) {
                if (channel.xSampling != slice.xSampling ||
                    channel.ySampling != slice.ySampling) {
                    throw new IllegalArgumentException(
                            "x and/or y subsampling factors of channel \"" +
                            name + "\" are not compatible with the " +
                            "frame buffer's subsampling factors.");
                }
            }
            
            // Incorporate the buffer's position into the slice offset: in the
            // native code we will get the buffer's address, which stays the
            // same even if the Java-side current position changes.
            Slice newSlice = new Slice(slice);
            newSlice.baseOffset += newSlice.buffer.position();
            fbValidatedCopy.insert(name, newSlice);            
        }
        return fbValidatedCopy;
    }
    
    
    
    /**
     * Initializes the required data structures on the native size. All other
     * native methods assume that this one has been already called.
     */
    private static native void initNativeCache();
    
    /**
     * Creates the underlying {@code Imf::FrameBuffer} frame buffer from a
     * non-empty list of (name,slice) pairs.
     * 
     * <p>The pairs to be added to the frame buffer are
     * {@code (names[i],slices[i])}.This method assumes that:
     * <ul>
     *   <li>{@code count} is greater than zero</li>
     *   <li>Both {@code names} and {@code slices} are non-null and have
     *   {@code count} elements, all of them non-null</li>
     * </ul></p>
     * @param count number of named slices
     * @param names names of the slices
     * @param slices description of the slices
     * @return a native pointer to the new {@code Imf::FrameBuffer} instance
     */
    private native static long newNativeFrameBuffer(int count,
            String[] names, Slice[] slices);

    /**
     * Deletes the native pointer previously created by
     * {@code newNativeFrameBuffer}.
     * @param nativeHandle the native {@code Imf::FrameBuffer} instance pointer
     */
    private native static void deleteNativeHandle(long nativeHandle);
    
    
    private final FrameBuffer fb;
    private final int fbHash;
    private long nativeHandle;
}

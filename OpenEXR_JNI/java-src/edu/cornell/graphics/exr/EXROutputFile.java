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

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

package edu.cornell.graphics.exr;

import edu.cornell.graphics.exr.attributes.PreviewImageAttribute;
import edu.cornell.graphics.exr.attributes.TypedAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.io.EXRByteArrayOutputStream;
import edu.cornell.graphics.exr.io.EXROutputStream;
import edu.cornell.graphics.exr.io.XdrOutput;
import java.nio.file.Path;
import java.util.Map;

/**
 * {@code EXROutputFile} provides an interface for writing scan line OpenEXR
 * images in a general way.
 * 
 * <p><b>Note that this implementation is not synchronized.</b> If multiple
 * threads access an {@code EXROutputFile} instance and at least one thread
 * changes the frame buffer of writes pixels, it <em>must</em> be
 * synchronized externally.</p>
 * 
 * @since OpenEXR-JNI 3.0
 */
public class EXROutputFile implements AutoCloseable {
    
    static {
        // TODO Unify native library loading
        System.loadLibrary("openexrjni");
    }
    
    /**
     * A constructor that opens the file denoted by the given path, writing
     * the given header.
     * 
     * <p>Calling {@code close} will close the file and destroy the underlying
     * native object. The image is written using the current number of threads
     * in the global pool.</p>
     * 
     * <p>Note that even though constructing an output file this way instead of
     * using an {@link EXROutputStream} may yield higher throughput, there is
     * a limitation on the number of files that can be simultaneously open.
     * On Windows 7 this limit is about 500.</p>
     * 
     * @param path the path to be opened for writing
     * @param hdr the header of the file
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(Path path, Header hdr) throws EXRIOException {
        this(path, hdr, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that opens the file denoted by the given path, writing
     * the given header, using a specific number of threads for I/O.
     * 
     * <p>Calling {@code close} will close the file and destroy the underlying
     * native object. {@code numThreads} determines the number of threads that
     * will be used to write and compress the file.</p>
     * 
     * <p>Note that even though constructing an output file this way instead of
     * using an {@link EXROutputStream} may yield higher throughput, there is
     * a limitation on the number of files that can be simultaneously open.
     * On Windows 7 this limit is about 500.</p>
     * 
     * @param path the path to be opened for writing
     * @param hdr the header of the file
     * @param numThreads number of native threads to write and compress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(Path path, Header hdr, int numThreads) 
            throws EXRIOException {
        if (path == null) {
            throw new IllegalArgumentException("null file path");
        }
        if (hdr == null) {
            throw new IllegalArgumentException("null header");
        }
        if (numThreads < 0) {
            throw new IllegalArgumentException("Invalid number of threads");
        }
        // This constructor uses a native stream, not the Java-based one
        stream = null;
        autoCloseStream = false;
        
        header = new Header(hdr);
        headerHashCode = header.hashCode();
        EXRByteArrayOutputStream hStream = serializeHeader(header);
        String filename = path.toAbsolutePath().toString();
        nativePtr = getNativeOutputFile(hStream.array(),(int)hStream.position(),
                null, filename, numThreads);
        assert nativePtr != 0L;
    }
    
    /**
     * A constructor that attaches the new {@code EXROutputStream} to a stream
     * that has already been open, writing the given header.
     * 
     * <p>Closing this instance will not close the stream, it will only destroy
     * the underlying native object. The image is written using the current
     * number of threads in the global pool.</p>
     * 
     * <p>Note that by using this constructor the writing performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param os an already open output stream
     * @param hdr the header of the file
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(EXROutputStream os, Header hdr) throws EXRIOException {
        this(os, hdr, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that attaches the new {@code EXROutputStream} to a stream
     * that has already been open, writing the given header, using a specific
     * number of threads for I/O.
     * 
     * <p>Closing this instance will not close the stream, it will only destroy
     * the underlying native object. {@code numThreads} determines the number of
     * threads that will be used to write and compress the file.</p>
     * 
     * <p>Note that by using this constructor the writing performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param os an already open output stream
     * @param hdr the header of the file
     * @param numThreads number of native threads to write and compress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(EXROutputStream os, Header hdr, int numThreads) 
            throws EXRIOException {
        this(os, hdr, false, numThreads);
    }
    
    /**
     * A constructor that attaches the new {@code EXROutputStream} to a stream
     * that has already been open, writing the given header, and allows to close
     * the output stream along with the output file.
     * 
     * <p>If {@code closeStream} is {@code true} closing this instance will
     * close the out stream as well (if it implements {@link AutoCloseable}),
     * otherwise the stream will remain open and it will only destroy the
     * underlying native object. The image is written using the current
     * number of threads in the global pool.</p>
     * 
     * <p>Note that by using this constructor the writing performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param os an already open output stream
     * @param hdr the header of the file
     * @param closeStream if {@code true} closing this output file will close
     *        the stream as well.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(EXROutputStream os, Header hdr, boolean closeStream) 
            throws EXRIOException {
        this(os, hdr, closeStream, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that attaches the new {@code EXROutputStream} to a stream
     * that has already been open, writing the given header, using a specific
     * number of threads for I/O and allows to close the output stream along
     * with the output file.
     * 
     * <p>If {@code closeStream} is {@code true} closing this instance will
     * close the out stream as well (if it implements {@link AutoCloseable}),
     * otherwise the stream will remain open and it will only destroy the
     * underlying native object. {@code numThreads} determines the number of
     * threads that will be used to write and compress the file.</p>
     * 
     * <p>Note that by using this constructor the writing performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param os an already open output stream
     * @param hdr the header of the file
     * @param closeStream if {@code true} closing this output file will close
     *        the stream as well.
     * @param numThreads number of native threads to write and compress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXROutputFile(EXROutputStream os, Header hdr, boolean closeStream,
            int numThreads) throws EXRIOException {
        if (os == null) {
            throw new IllegalArgumentException("null output stream");
        }
        if (hdr == null) {
            throw new IllegalArgumentException("null header");
        }
        if (closeStream && !(os instanceof AutoCloseable)) {
            throw new IllegalArgumentException("the stream is not closeable");
        }
        if (numThreads < 0) {
            throw new IllegalArgumentException("Invalid number of threads");
        }
        
        stream = os;
        autoCloseStream = closeStream;
        header = new Header(hdr);
        headerHashCode = header.hashCode();
        EXRByteArrayOutputStream hStream = serializeHeader(header);
        nativePtr = getNativeOutputFile(hStream.array(),(int)hStream.position(),
                os, null, numThreads);
        assert nativePtr != 0L;
    }
    
    /** 
     * Writes the magic number, version and serialized header into a newly
     * allocated direct buffer.
     */
    private static EXRByteArrayOutputStream serializeHeader(Header header) 
            throws EXRIOException {
        EXRByteArrayOutputStream stream = new EXRByteArrayOutputStream();
        XdrOutput output = new XdrOutput(stream);
        
        output.writeInt(EXRVersion.MAGIC);
        output.writeInt(header.version());
        header.writeTo(output);
        return stream;
    }
    
    /**
     * Closes the underlying stream, if one was used when the instance was
     * constructed, and destroys the underlying native object. Calling this
     * method multiple times has no effect.
     * 
     * @throws EXRIOException if there is an error while closing either the
     *         output file or the underlying stream 
     */
    @Override
    public void close() throws EXRIOException {
        if (nativePtr != 0L) {
            try {
                deleteNativeOutputFile(nativePtr);
                nativePtr = 0L;
                if (stream != null && autoCloseStream) {
                    assert stream instanceof AutoCloseable;
                    ((AutoCloseable) stream).close();
                }
            } catch (Exception ex) {
                throw new EXRIOException(ex.getMessage(), ex);
            }
        }
    }
    
    /**
     * Returns {@code true} if the underlying input file is open for writing.
     * @return {@code true} if the underlying input file is open for writing.
     */
    public boolean isOpen() {
        return nativePtr != 0L;
    }
    
    /**
     * Returns a reference to the output file's header.
     * 
     * <p>For efficiency this function returns the actual header of this output
     * file thus callers ought <em>not</em> to modify the returned instance.
     * In contrast to C++, returning a completely read-only header would require
     * a lot of changes, including supporting only attributes with all-final
     * accessible fields and/or read-only aware setters. As a safety feature
     * the implementation caches the hash code of the header upon creating the
     * file and compares it against the current header's hash code. It they
     * do not match it throws an {@code IllegalStateException}.</p>
     * 
     * <p>Because of the validation, clients which will use the header for
     * multiple operations should call {@code getHeader} only once and then
     * use the validated reference.</p>
     * 
     * @return a reference to the output file's header.
     * @throws IllegalStateException if the header has been modified externally
     */
    public Header getHeader() {
        if (headerHashCode != header.hashCode()) {
            throw new IllegalStateException("the header has been modified");
        }
        return header;
    }
    
    /**
     * Set the current frame buffer by doing a deep copy of the FrameBuffer
     * input object into this instance.
     * 
     * <p> The current frame buffer is the source for the pixel
     * data written to the stream.  The current frame buffer must be
     * set at least once before {@code writeixels()} is called.
     * The current frame buffer can be changed after each call
     * to {@code writePixels()}.</p>
     * 
     * <p>Note that there is no automatic conversion of neither pixel types
     * or sampling factors for output files: for each slice with a 
     * matching channel in the header, their pixel types and x/y sampling
     * factor must match.</p>
     * 
     * @param fBuffer the source frame buffer
     * @throws IllegalArgumentException if {@code fBuffer} does not
     *         meet the required preconditions
     * @throws IllegalStateException if the stream is not open or the header was
     *         modified externally
     */
    public void setFrameBuffer(FrameBuffer fBuffer) {
        if (!isOpen()) {
            throw new IllegalStateException("the file is already closed");
        }
        if (headerHashCode != header.hashCode()) {
            throw new IllegalStateException("the header has been modified");
        }
        if (fBuffer == null) {
            throw new IllegalArgumentException("null frame buffer");
        } else if (fBuffer.isEmpty()) {
            throw new IllegalArgumentException("empty frame buffer");
        }
        
        ChannelList channels = header.getChannels();
        for (Map.Entry<String, Slice> fbEntry : fBuffer) {
            Channel channel = channels.findChannel(fbEntry.getKey());
            if (channel != null && channel.type != fbEntry.getValue().type) {
                throw new IllegalArgumentException("Pixel type mismatch for " +
                        "slice \"" + fbEntry.getKey() + "\": expected " +
                        channel.type + ", actual " + fbEntry.getValue().type);
            }
        }
        
        nativeFrameBuffer = new NativeFrameBuffer(fBuffer,header.getChannels());
        setNativeFrameBuffer(nativePtr, nativeFrameBuffer.getNativeHandle());
        // setNativeFrameBuffer creates an internal copy of the frame buffer,
        // so we can delete the current native copy
        nativeFrameBuffer.deleteNativeHandle();
    }
    
    /**
     * Returns a reference to the current frame buffer, or {@code null} if it
     * has not been set yet.
     * 
     * <p>For efficiency this function returns the actual frame buffer of this
     * output file, thus callers ought <em>not</em> to modify the returned
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
     * @return a reference to the current frame buffer, or {@code null} if it
     *         has not been set yet
     * @throws IllegalStateException if the frame buffer has been modified
     *         externally
     */
    public FrameBuffer getFrameBuffer() {
        if (nativeFrameBuffer != null) {
            return nativeFrameBuffer.getFrameBuffer();
        } else {
            return null;
        }
    }
    
    /**
     * Write pixel data for the next group of consecutive scan lines.
     * 
     * <p>{@code writePixels(n)} retrieves the next {@code n} scan lines worth
     * of data from the current frame buffer, starting with the scan line 
     * indicated by {@code currentScanLine()}, and stores the data in the
     * output file, and progressing in the direction indicated by
     * {@code header.lineOrder()}.</p>
     * 
     * <p>To produce a complete and correct file, exactly {@code m} scan lines
     * must be written, where {@code m} is equal to:
     * <pre>  header().dataWindow().max.y - header().dataWindow().min.y + 1</pre>
     * </p>
     * 
     * @param numScanlines number of the next consecutive scan lines to write
     * @throws IllegalArgumentException if {@code numScanLines} is less than
     *         {@literal 1}
     * @throws IndexOutOfBoundsException
     *        if {@code (currentScanline() + numScanlines - 1)} is outside 
     *        {@code [header().dataWindow().min.y, header().dataWindow().max.y]}
     * @throws IllegalStateException if the file is not open for reading,
     *        or if the frame buffer has not been set, or if either the header
     *        or the current frame buffer have been modified externally
     * @see #setFrameBuffer(edu.cornell.graphics.exr.FrameBuffer) 
     */
    public void writePixels(int numScanlines) throws IllegalArgumentException,
            IndexOutOfBoundsException, IllegalStateException {
        if (numScanlines < 1) {
            throw new IllegalArgumentException(
                    "invalid number of scan lines: " + numScanlines);
        } else if (!isOpen()) {
            throw new IllegalStateException("the file is already closed");
        } else if (nativeFrameBuffer == null) {
            throw new IllegalStateException("the input file does not have a " +
                    "valid frame buffer yet");
        } else if (headerHashCode != header.hashCode()) {
            throw new IllegalStateException("the header has been modified");
        }
        
        // Verify that reads will stay within each buffer valid range
        final Box2<Integer> dw = header.getDataWindow();
        final int scanLine1 = currentScanLine();
        final int scanLine2 = scanLine1 + numScanlines - 1;
        nativeFrameBuffer.validateForDataWindow(dw, scanLine1, scanLine2);
        writeNativePixels(nativePtr, numScanlines);
    }
    
    /**
     * Write the next scan line from the frame buffer.
     * 
     * <p>This implementations simply calls {@code writePixels(1)}</p>
     * @see #writePixels(int) 
     */
    public void writePixels() {
        writePixels(1);
    }
    
    /**
     * Returns the current scan line.
     * 
     * <p>{@code currentScanLine()} returns the {@code y} coordinate of the
     * first scan line that will be read from the current frame buffer during
     * the next call to writePixels().</p>
     * 
     * <p>If {@code header.lineOrder() == INCREASING_Y}:<br/>
     * The current scan line before the first call to {@code writePixels()}
     * is {@code header().dataWindow().min.y}.  After writing each scan line,
     * the current scan line is incremented by {@literal 1}.</p>
     * 
     * <p>If {@code header.lineOrder() == DECREASING_Y}:<br/>
     * The current scan line before the first call to {@code writePixels()}
     * is {@code header().dataWindow().max.y}.  After writing each scan line,
     * the current scan line is decremented by {@literal 1}.
     * 
     * @return the current scan line
     * @throws IllegalStateException if the file is already closed
     */
    public int currentScanLine() {
        if (isOpen()) {
            return currentNativeScanLine(nativePtr);
        } else {
            throw new IllegalStateException("the file is already closed");
        }
    }
    
    /**
     * Updates the preview image attribute, it the header has such attribute.
     * 
     * <p>{@code updatePreviewImage()} supplies a new set of pixels for the
     * preview image attribute in the file's header.  If the header
     * does not contain a preview image, {@code updatePreviewImage()} throws
     * an {@code IllegalArgumentException}.</p>
     * 
     * <p>Note: {@code updatePreviewImage()} is necessary because images are
     * often stored in a file incrementally, a few scan lines at a
     * time, while the image is being generated.  Since the preview
     * image is an attribute in the file's header, it gets stored in
     * the file as soon as the file is opened, but we may not know
     * what the preview image should look like until we have written
     * the last scan line of the main image.</p>
     * 
     * @param newPixels the new interleaved {@code RGBA8} pixels
     * @throws IllegalArgumentException if the header does not have a preview
     *         image attribute, or {@code newPixels} is either null or
     *         does not have enough data
     */
    public void updatePreviewImage(byte[] newPixels) {
        TypedAttribute<PreviewImage> previewAttr = header.findTypedAttribute(
                "preview", PreviewImageAttribute.class);
        if (previewAttr == null) {
            throw new IllegalArgumentException("missing preview attribute");
        }
        final PreviewImage preview = previewAttr.getValue();
        final int numPixels = preview.width * preview.height;
        if (newPixels == null || newPixels.length < numPixels) {
            throw new IllegalArgumentException("invalid newPixels buffer");            
        }
        
        throw new UnsupportedOperationException("Not supported yet.");
    }
    
    
    
    /**
     * Return a native pointer, stored inside a long, of a new native 
     * {@code Imf::OutputFile} instance created from either a non-null
     * input stream or opening a file. Exactly one of the arguments has to be
     * non-null.
     * 
     * @param headerBuffer byte array with the OpenEXR magic
     *        number, the header's version number and the serialized header
     * @param len number of valid bytes in {@code headerBuffer}
     * @param os output stream to use, or {@code null} if using a filename
     * @param fname filename to use, or {@code null} if using a stream
     * @param numThreads 
     * @return a native pointer as a long.
     */
    private static native long getNativeOutputFile(byte[] headerBuffer, int len,
            EXROutputStream os, String fname, int numThreads);
    
    /**
     * Deletes a native output file previously created by
     * {@link #getNativeOutputFile(ByteBuffer, EXROutputStream, String, int)  }.
     * 
     * @param nativePointer handle to the underlying native code
     */
    private static native void deleteNativeOutputFile(long nativePointer);
    
    /**
     * Sets the underlying frame buffer using a previously created native
     * {@code Imf::FrameBuffer} instance.
     * 
     * <p>This method assumes that:
     * <ul>
     *   <li>{@code nativePointer} is the value returned by
     *   {@code getNativeInputFile}</li>
     *   <li>{@code nativeFrameBufferPointer} is the value returned by
     *   {@link NativeFrameBuffer#getNativeHandle() }</li>
     * </ul></p>
     * @param nativePointer handle to the underlying native code
     * @param nativeFrameBufferPointer handle to the {@code Imf::FrameBuffer}
     *        instance
     */
    private static native void setNativeFrameBuffer(long nativePointer,
            long nativeFrameBufferPointer);
    
    /**
     * Returns the current scan line from the underlying native instance.
     * 
     * <p>This method assumes that {@code nativePointer} is the value returned
     * by {@code getNativeInputFile}</p>
     * 
     * @param nativePointer handle to the underlying native code
     * @return the current scan line from the underlying native instance
     */
    private static native int currentNativeScanLine(long nativePointer);
    
    /**
     * Writes the next group of consecutive scan lines from the current
     * frame buffer into the underlying native output file.
     * 
     * <p>This method assumes that:
     * <ul>
     *   <li>{@code nativePointer} is the value returned by
     *   {@code getNativeInputFile}</li>
     *   <li>{@code numScanlines} has a valid value</li>
     *   <li>A previous call to {@link #setFrameBuffer(FrameBuffer) }
     *   completed successfully.</li>
     * </ul></p>
     * 
     * @param nativePointer
     * @param numScanlines 
     */
    private static native void writeNativePixels(long nativePointer,
            int numScanlines);
    
    
    
    /** Optional reference to the EXR output stream being used */
    private final EXROutputStream stream;
    
    /** Flag to close the EXROutputStream along with the output file */
    private final boolean autoCloseStream;
    
    /** Native pointer to the underlying class */
    private long nativePtr = 0L;
    
    /** File header written to the output file */
    private final Header header;
    
    /** Cached hash code of the original header */
    private final int headerHashCode;
    
    /** Local frame buffer */
    private NativeFrameBuffer nativeFrameBuffer = null;
}

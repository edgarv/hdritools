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

import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.io.ByteBufferInputStream;
import edu.cornell.graphics.exr.io.EXRFileInputStream;
import edu.cornell.graphics.exr.io.EXRInputStream;
import edu.cornell.graphics.exr.io.XdrInput;
import java.nio.ByteBuffer;
import java.nio.file.Path;

/**
 * {@code EXRInputFile} is scanline-based interface that can be used to read
 * both scanline-based and tiled OpenEXR image files.
 * 
 * <p><b>Note that this implementation is not synchronized.</b> If multiple
 * threads access an {@code EXRInputFile} instance and at least one thread
 * changes the frame buffer of reads pixels, it <em>must</em> be
 * synchronized externally.</p>
 */
public class EXRInputFile implements AutoCloseable {
    
    static {
        // TODO Unify native library loading
        System.loadLibrary("openexrjni");
    }

    /**
     * A constructor that opens the file denoted by the given path.
     * 
     * <p>Calling {@code close} will close the file and destroy the underlying
     * native object. The image is read using the current number of threads
     * in the global pool.</p>
     * 
     * <p>Note that even though constructing an input file this way instead of
     * using an {@link EXRInputStream} may yield higher throughput, there is
     * a limitation on the number of files that can be simultaneously open.
     * On Windows 7 this limit is about 500.</p>
     * 
     * @param path the path to be opened for reading
     * @throws EXRIOException if an I/O error occurs
     * @see Threading 
     */
    public EXRInputFile(Path path) throws EXRIOException {
        this(path, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that opens the file denoted by the given path, using a 
     * specific number of threads for I/O.
     * 
     * <p>Calling {@code close} will close the file and destroy the underlying
     * native object. {@code numThreads} determines the number of threads that
     * will be used to read and decompress the file.</p>
     * 
     * <p>Note that even though constructing an input file this way instead of
     * using an {@link EXRInputStream} may yield higher throughput, there is
     * a limitation on the number of files that can be simultaneously open.
     * On Windows 7 this limit is about 500.</p>
     * 
     * @param path the path to be opened for reading
     * @param numThreads number of native threads to read and decompress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading
     */
    public EXRInputFile(Path path, int numThreads) throws EXRIOException {
        if (path == null) {
            throw new IllegalArgumentException("null file path");
        }
        if (numThreads < 0) {
            throw new IllegalArgumentException("Invalid number of threads");
        }
        // This constructor uses a native stream, not the Java-based one
        stream = null;
        autoCloseStream = false;
        
        String filename = path.toAbsolutePath().toString();
        nativePtr = getNativeInputFile(null, filename, numThreads);
        assert nativePtr != 0L;
        version  = getNativeVersion(nativePtr);
        complete = isNativeComplete(nativePtr);
        header   = initHeader(nativePtr, version);
        headerHashCode = header.hashCode();
    }
    
    /**
     * A constructor that attaches the new {@code EXRInputFile} to a stream
     * that has already been opened.
     * 
     * <p>Closing this instance will not close the
     * stream, it will only destroy the underlying native object.
     * The image is read using the current number of threads in the global pool.
     * </p>
     * 
     * <p>Note that by using this constructor the reading performance may be
     * lower than that attained by the constructors which explicitly get a pah
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param is an already open input stream
     * @throws EXRIOException if an I/O error occurs
     * @see Threading
     */
    public EXRInputFile(EXRFileInputStream is) throws EXRIOException {
        this(is, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that attaches the new {@code EXRInputFile} to a stream
     * that has already been opened, using a specific number of threads for I/O.
     * 
     * <p>Closing this instance will not close the
     * stream, it will only destroy the underlying native object.
     * {@code numThreads} determines the number of threads that will be used to
     * read and decompress the file.</p>
     * 
     * <p>Note that by using this constructor the reading performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param is an already open input stream
     * @param numThreads number of native threads to read and decompress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading
     */
    public EXRInputFile(EXRFileInputStream is, int numThreads)
            throws EXRIOException {
       this(is, false, numThreads); 
    }
    
    /**
     * A constructor that attaches the new {@code EXRInputFile} to a stream
     * that has already been opened and allows to close the input stream along
     * with the input file.
     * 
     * <p>If {@code closeStream} is {@code true} closing this instance will
     * close the input stream as well (if it implements {@link AutoCloseable}),
     * otherwise the stream will remain open and it will only destroy the
     * underlying native object. The image is read using the current number of
     * threads in the global pool.</p>
     * 
     * <p>Note that by using this constructor the reading performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param is an already open input stream
     * @param closeStream if {@code true} closing this input file will close
     *        the stream as well.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading
     */
    public EXRInputFile(EXRFileInputStream is, boolean closeStream)
            throws EXRIOException {
        this(is, closeStream, Threading.globalThreadCount());
    }
    
    /**
     * A constructor that attaches the new {@code EXRInputFile} to a stream
     * that has already been opened, using a specific number of threads for I/O
     * and allows to close the input stream along with the input file.
     * 
     * <p>If {@code closeStream} is {@code true} closing this instance will
     * close the input stream as well (if it implements {@link AutoCloseable}),
     * otherwise the stream will remain open and it will only destroy the
     * underlying native object.
     * {@code numThreads} determines the number of threads that will be used to
     * read and decompress the file.</p>
     * 
     * <p>Note that by using this constructor the reading performance may be
     * lower than that attained by the constructors which explicitly get a path
     * to open. This is due to the current implementation which relies on JNI
     * calls to the OpenEXR C++ library.</p>
     * 
     * @param is an already open input stream
     * @param closeStream if {@code true} closing this input file will close
     *        the stream as well.
     * @param numThreads number of native threads to read and decompress the
     *        file, if zero all tasks run on the current thread.
     * @throws EXRIOException if an I/O error occurs
     * @see Threading
     */
    public EXRInputFile(EXRFileInputStream is, boolean closeStream,
            int numThreads) throws EXRIOException {
        if (is == null) {
            throw new IllegalArgumentException("null input stream");
        }
        if (closeStream && !(is instanceof AutoCloseable)) {
            throw new IllegalArgumentException("the stream is not closeable");
        }
        if (numThreads < 0) {
            throw new IllegalArgumentException("Invalid number of threads");
        }
        stream = is;
        autoCloseStream = closeStream;
        nativePtr = getNativeInputFile(is, null, numThreads);
        assert nativePtr != 0L;
        version  = getNativeVersion(nativePtr);
        complete = isNativeComplete(nativePtr);
        header   = initHeader(nativePtr, version);
        headerHashCode = header.hashCode();
    }
    
    /** Create a new header from the native input file */
    private static Header initHeader(long nativePtr, int version) 
            throws EXRIOException {
        byte[] data = getNativeHeaderBuffer(nativePtr);
        if (data == null || data.length == 0) {
            throw new IllegalStateException("invalid header buffer");
        }

        XdrInput input =
                new XdrInput(new ByteBufferInputStream(ByteBuffer.wrap(data)));
        Header header = new Header();
        header.readFrom(input, version);
        return header;
    }
    
    /**
     * Closes the underlying file, it one was opened when the instance was
     * constructed, and destroys the underlying native object. Calling this
     * method multiple times has no effect.
     * 
     * @throws EXRIOException if there is an error while closing either the
     *         input file or the underlying stream
     */
    @Override
    public void close() throws EXRIOException {
        if (nativePtr != 0L) {
            try {
                deleteNativeInputFile(nativePtr);
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
     * Returns {@code true} if the underlying input file is open for reading.
     * @return {@code true} if the underlying input file is open for reading.
     */
    public boolean isOpen() {
        return nativePtr != 0L;
    }
    
    /**
     * Returns a reference to the input file's header.
     * 
     * <p>For efficiency this function returns the actual header of this input
     * file thus callers ought <em>not</em> to modify the returned instance.
     * In contrast to C++, returning a completely read-only header would require
     * a lot of changes, including supporting only attributes with all-final
     * accessible fields and/or read-only aware setters. As a safety feature
     * the implementation caches the hash code of the header upon opening the
     * file and compares it against the current header's hash code. It they
     * do not match it throws an {@code IllegalStateException}.</p>
     * 
     * <p>Because of the validation, clients which want to use the header for
     * multiple operations should call {@code getHeader} only once and then
     * use the validated reference.</p>
     * 
     * @return a reference to the input file's header.
     * @throws IllegalStateException if the header has been modified externally
     */
    public Header getHeader() {
        if (headerHashCode != header.hashCode()) {
            throw new IllegalStateException("the header has been modified");
        }
        return header;
    }
    
    /**
     * Returns the file format version as read from the file.
     * @return the file format version as read from the file
     */
    public int getVersion() {
        return version;
    }
    
    /**
     * Set the current frame buffer by doing a deep copy of the FrameBuffer
     * input object into the InputFile object.
     * 
     * <p> The current frame buffer is the destination for the pixel
     * data read from the file.  The current frame buffer must be
     * set at least once before {@code readPixels()} is called.
     * The current frame buffer can be changed after each call
     * to {@code readPixels()}.
     * 
     * @param fBuffer the source frame buffer
     * @throws IllegalArgumentException if the input frame buffer does not
     *         meet the required preconditions
     * @throws IllegalStateException if the file is not open or the header was
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
     * input file, thus callers ought <em>not</em> to modify the returned
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
     * has not been set yet
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
     * Returns {@code true} if the file is complete.
     *
     * <p>{@code isComplete()} returns {@code true} if all pixels in the data
     * window are present in the input file, or {@code false} if any pixels are
     * missing. (Another program may still be busy writing the file, or file
     * writing may have been aborted prematurely.)</p>
     *
     * @return {@code true} if the file is complete
     */
    public boolean isComplete() {
        return complete;
    }
    
    /**
     * Read pixel data from a scan line range into the current frame buffer.
     *
     * <p>{@code readPixels(s1,s2)} reads all scan lines with {@code y}
     * coordinates in the interval {@code [min (s1, s2), max (s1, s2)]} from the
     * file, and stores them in the current frame buffer. Both {@code s1}
     * and {@code s2} must be within the interval
     * {@code [header().dataWindow().min.y, header().dataWindow().max.y]}
     *
     * <p>The scan lines can be read from the file in random order, and
     * individual scan lines may be skipped or read multiple times.
     * For maximum efficiency, the scan lines should be read in the
     * order in which they were written to the file.</p>
     * 
     * @param scanLine1 first scan line to read
     * @param scanLine2 last scan line to read
     * @throws IndexOutOfBoundsException 
     *        if either {@code s1} or {@code s2} are outside the interval
     *        {@code [header().dataWindow().min.y, header().dataWindow().max.y]}
     * @throws IllegalStateException if the file is not open for reading,
     *        or if the frame buffer has not been set, or if either the header
     *        or the current frame buffer have been modified externally
     * @see #setFrameBuffer(edu.cornell.graphics.exr.FrameBuffer) 
     */
    public void readPixels(int scanLine1, int scanLine2) throws
            IndexOutOfBoundsException, IllegalStateException {
        if (!isOpen()) {
            throw new IllegalStateException("the file is already closed");
        } else if (nativeFrameBuffer == null) {
            throw new IllegalStateException("the input file does not have a " +
                    "valid frame buffer yet");
        } else if (headerHashCode != header.hashCode()) {
            throw new IllegalStateException("the header has been modified");
        }
        
        // Verify that writes will stay within each buffer valid range
        final Box2<Integer> dw = header.getDataWindow();
        nativeFrameBuffer.validateForDataWindow(dw, scanLine1, scanLine2);
        readNativePixels(nativePtr, scanLine1, scanLine2);
    }
    
    /**
     * Read pixel data from a single scan line into the current frame buffer.
     * 
     * <p>This implementation simply calls {@code readPixels(s,s)}</p>
     * 
     * @param s scan line to read
     * @see #readPixels(int, int) 
     */
    public void readPixels(int s) {
        readPixels(s, s);
    }
    
    
    
    /**
     * Return a native pointer, stored inside a long, of a new native 
     * {@code Imf::InputFile} instance created from either a non-null
     * input stream or opening a file. Exactly one of the arguments has to be
     * non-null.
     * 
     * @param is input stream to use, or {@code null} if using a file
     * @param fname filename to use, or {@code null} if using a stream
     * @param numThreads 
     * @return a native pointer as a long.
     */
    private static native long getNativeInputFile(EXRInputStream is,
            String fname, int numThreads);
    
    /**
     * Returns the version number of the native input file.
     * @param nativePointer handle to the underlying native code
     * @return the version number of the native input file
     */
    private static native int getNativeVersion(long nativePointer);
    
    /**
     * Returns {@code true} if the native input file is complete.
     * @param nativePointer handle to the underlying native code
     * @return {@code true} if the native input file is complete
     */
    private static native boolean isNativeComplete(long nativePointer);
    
    /**
     * Returns the serialized representation of the native input file header.
     * 
     * <p>This receives a handle previously created by
     * {@link #getNativeInputFile(EXRInputStream, String, int) } and the 
     * returned byte array contents are in the format used by
     * {@link Header#readFrom(edu.cornell.graphics.exr.io.XdrInput, int) }
     * 
     * @param nativePointer handle to the underlying native code
     * @return the serialized representation of the native input file header.
     */
    private static native byte[] getNativeHeaderBuffer(long nativePointer);
    
    /**
     * Deletes a native input file previously created by
     * {@link #getNativeInputFile(EXRInputStream, String, int) } .
     * 
     * @param nativePointer handle to the underlying native code
     */
    private static native void deleteNativeInputFile(long nativePointer);
    
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
     * Reads a group of consecutive scan lines from the underlying native input
     * file into the current frame buffer.
     * 
     * <p>This method assumes that:
     * <ul>
     *   <li>{@code nativePointer} is the value returned by
     *   {@code getNativeInputFile}</li>
     *   <li>Both {@code scanLine1} and {@code scanLine2} are within 
     *   {@code [header().dataWindow().min.y, header().dataWindow().max.y]}</li>
     *   <li>A previous call to {@link #setFrameBuffer(FrameBuffer) }
     *   completed successfully.</li>
     * </ul></p>
     * 
     * @param nativePointer
     * @param scanLine1
     * @param scanLine2 
     */
    private static native void readNativePixels(long nativePointer,
            int scanLine1, int scanLine2);
    
    
    /** Optional reference to the EXRInput stream being used */
    private final EXRInputStream stream;
    
    /** Flag to close the EXRInputStream along with the input file */
    private final boolean autoCloseStream;
    
    /** Native pointer to the underlying class */
    private long nativePtr = 0L;
    
    /** File header read from the input file */
    private final Header header;
    
    /** Cached hash code of the input file */
    private final int headerHashCode;
    
    /** Version number read from the input file */
    private final int version;
    
    /** Complete status of the input file */
    private final boolean complete;
    
    /** Local frame buffer */
    private NativeFrameBuffer nativeFrameBuffer = null;
}

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

package edu.cornell.graphics.exr.io;

import edu.cornell.graphics.exr.EXRIOException;
import java.nio.ByteBuffer;

/**
 * Low-level EXR output stream interface.
 * 
 * @since OpenEXR-JNI 3.0
 */
public interface EXROutputStream {
    
    /**
     * Write to the stream.
     * 
     * <p>{@code write(dst)} writes <em>n</em> bytes to the stream, where
     * <em>n</em> is the number of bytes remaining in the buffer, that is,
     * {@code dst.remaining()}, at the moment this method is invoked. The bytes
     * are written from the current position of the buffer.</p>
     * 
     * <p>If an I/O error occurs, {@code write(dst)} throws an exception.</p>
     * 
     * @param src The buffer from which bytes are to be transferred 
     * @throws EXRIOException if an I/O error occurs.
     */
    void write(ByteBuffer src) throws EXRIOException;
   
    /**
     * Get the current writing position, in bytes from the beginning of the
     * stream. If the next call to {@code write()} will write the first byte in
     * the stream, {@code position()} returns {@literal 0}.
     * 
     * @return the current writing position, in bytes from the beginning of the
     *         stream.
     * @throws EXRIOException if an I/O error occurs.
     */
    long position() throws EXRIOException;
    
    /**
     * Set the current position. After calling {@code position(i)},
     * {@code position()} returns <em>i</em>.
     * 
     * @param pos the new writing position.
     * @return a reference to this stream
     * @throws EXRIOException if an I/O error occurs.
     */
    EXROutputStream position(long pos) throws EXRIOException;
}

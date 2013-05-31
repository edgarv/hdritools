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
 * Low-level EXR input stream interface.
 * 
 * @since 0.3
 */
public interface EXRInputStream {
    
    /**
     * Read from the stream.
     * 
     * <p>{@code read(dst)} reads <em>n</em> bytes from the stream, where
     * <em>n</em> is the number of bytes remaining in the buffer, that is,
     * {@code dst.remaining()}, at the moment this method is invoked. The bytes
     * are read into the current position of the buffer.</p>
     * 
     * <p>If the stream contains less than <em>n</em> bytes, or if an I/O error
     * occurs, {@code read(dst)} throws an exception. If {@code read(dst)}
     * reads the last byte from the stream it returns {@code false}, otherwise
     * it returns {@code true}.</p>
     * 
     * @param dst The buffer into which bytes are to be transferred 
     * @return {@code false} if the call read the last byte from the stream,
     *         {@code true} otherwise.
     * @throws EXRIOException if the stream contains less than <em>n</em>
     *         bytes, or if an I/O error occurs.
     */
    boolean read(ByteBuffer dst) throws EXRIOException;
    
    /**
     * Get the current reading position, in bytes from the beginning of the
     * stream. If the next call to {@code read()} will read the first byte in
     * the stream file, {@code position()} returns {@literal 0}.
     * 
     * @return the current reading position, in bytes from the beginning of the
     *         stream.
     * @throws EXRIOException if an I/O error occurs.
     */
    long position() throws EXRIOException;
    
    /**
     * Set the current position. After calling {@code position(i)},
     * {@code position()} returns <em>i</em>.
     * 
     * @param pos the new reading position.
     */
    void position(long pos) throws EXRIOException;
    
}

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
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.nio.ReadOnlyBufferException;

/**
 * Input stream implementation using an existing byte buffer
 */
public class ByteBufferInputStream implements EXRInputStream {
    
    private final ByteBuffer buffer;
    private final int originalPosition;
    
    /**
     * Creates a new {@code ByteBufferInputStream} by wrapping the input
     * non null buffer.
     * 
     * <p>The {@code position} methods of this stream are relative to the
     * initial value of {@code buffer.position()}.
     * 
     * @param buffer the source byte buffer
     * @throws IllegalArgumentException if {@code buffer} is {@code null}.
     */
    public ByteBufferInputStream(ByteBuffer buffer) {
        if (buffer == null) {
            throw new IllegalArgumentException("null buffer");
        }
        this.buffer = buffer;
        this.originalPosition = buffer.position();
    }

    @Override
    public boolean read(ByteBuffer dst) throws EXRIOException {
        final int oldLimit = buffer.limit();
        try {
            buffer.limit(buffer.position() + dst.remaining());
            assert buffer.remaining() == dst.remaining();
            dst.put(buffer);
        } catch (IllegalArgumentException | 
                 BufferOverflowException  | 
                 ReadOnlyBufferException ex) {
            throw new EXRIOException(ex.getMessage(), ex);
        } finally {
            buffer.limit(oldLimit);
        }
        return buffer.hasRemaining();
    }

    @Override
    public long position() throws EXRIOException {
        int pos = buffer.position() - originalPosition;
        return pos;
    }

    @Override
    public void position(long pos) throws EXRIOException {
        int newPos = (int) (pos + originalPosition);
        try {
            buffer.position(newPos);
        } catch (IllegalArgumentException ex) {
            throw new EXRIOException(ex.getMessage(), ex);
        }
    }
    
}

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
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

/**
 * This class implements an EXR output stream in which the data is written into
 * a byte array. The buffer automatically grows as data is written to it.
 * The backing array can be retrieved using {@code array()}
 */
public class EXRByteArrayOutputStream implements EXROutputStream {
    
    private static class HelperStream extends ByteArrayOutputStream {
        HelperStream(int size) {
            super(size);
        }
        
        void position(int pos) {
            assert 0 <= pos && pos <= count;
            count = pos;
        }
        
        byte[] array() {
            return buf;
        }
        
        int capacity() {
            return buf.length;
        }
    }
    
    private final HelperStream stream;
       
    /**
     * Creates a new byte array output stream, with a buffer capacity of
     * the specified size, in bytes.
     *
     * @param size the initial size
     * @throws IllegalArgumentException if size is negative
     */
    public EXRByteArrayOutputStream(int size) {
        if (size < 0) {
            throw new IllegalArgumentException("Negative initial size: "
                                               + size);
        }
        stream = new HelperStream(size);
    }
    
    /**
     * Creates a new byte array output stream, with an initial buffer capacity
     * of 512 bytes.
     */
    public EXRByteArrayOutputStream() {
        this(512);
    }
    
    /**
     * Returns the byte array that backs this output stream.
     * 
     * <p>The array contains valid data in the interval {@code [0, position())}.
     * Modifications to this buffer's content will cause the returned array's
     * content to be modified, and vice versa.</p>
     * 
     * @return the byte array that backs this output stream
     */
    public byte[] array() {
        return stream.array();
    }

    @Override
    public void write(ByteBuffer src) throws EXRIOException {
        try {
            if (src.hasArray()) {
                final int off = src.arrayOffset() + src.position();
                stream.write(src.array(), off, src.remaining());
                src.position(src.limit());
            } else {
                while (src.hasRemaining()) {
                    stream.write(src.get());
                }
            }
        } catch (Exception e) {
            throw new EXRIOException(e.getMessage(), e);
        }
    }

    @Override
    public long position() throws EXRIOException {
        return stream.size();
    }

    @Override
    public EXROutputStream position(long pos) throws EXRIOException {
        if (pos < 0 || pos > stream.capacity()) {
            throw new EXRIOException("Invalid position: " + pos);
        }
        stream.position((int) pos);
        return this;
    }
    
}

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
import java.nio.ByteOrder;
import java.nio.charset.Charset;

/**
 * Helper class which encapsulates an {@code EXRInputStream} to read primitive
 * and string data.
 * 
 * <p>Each instance uses a private buffer to read data from the stream. All
 * methods are <em>not</em> thread safe, and assume that the stream will remain
 * valid through the lifetime of a {@code XdrInput} instance.</p>
 * 
 * <p>Modeled after the functions in ImfXdr.h from the C++ OpenEXR library.</p>
 * 
 * @see EXRInputStream
 * @since OpenEXR-JNI 3.0
 */
public class XdrInput {
    
    private final static int BUFFER_LEN = 512;
    
    /** Actual instance of the UTF8 charset to encode/decode strings */
    private final Charset UTF8 = Charset.forName("UTF-8");
    
    private final ByteBuffer buffer;
    private final EXRInputStream is;
    
    
    public XdrInput(EXRInputStream stream) {
        if (stream == null) {
            throw new IllegalArgumentException("null input stream");
        }
        buffer = ByteBuffer.allocate(BUFFER_LEN);
        if (!buffer.hasArray()) {
            throw new IllegalStateException("buffer not backed by an array");
        }
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        is = stream;
    }
    
    public long position() throws EXRIOException {
        return is.position();
    }
    
    public boolean readBoolean() throws EXRIOException {
        return readByte() != 0;
    }
    
    public byte readByte() throws EXRIOException {
        buffer.rewind();
        buffer.limit(1);
        is.read(buffer);
        return buffer.get(0);
    }
    
    public int readUnsignedByte() throws EXRIOException {
        return readByte() & 0xff;
    }
    
    public short readShort() throws EXRIOException {
        buffer.rewind();
        buffer.limit(2);
        is.read(buffer);
        return buffer.getShort(0);
    }
    
    public int readUnsignedShort() throws EXRIOException {
        return readShort() & 0xffff;
    }
    
    public int readInt() throws EXRIOException {
        buffer.rewind();
        buffer.limit(4);
        is.read(buffer);
        return buffer.getInt(0);
    }
    
    public long readLong() throws EXRIOException {
        buffer.rewind();
        buffer.limit(8);
        is.read(buffer);
        return buffer.getLong(0);
    }
    
    public float readFloat() throws EXRIOException {
        buffer.rewind();
        buffer.limit(4);
        is.read(buffer);
        return buffer.getFloat(0);
    }
    
    public double readDouble() throws EXRIOException {
        buffer.rewind();
        buffer.limit(8);
        is.read(buffer);
        return buffer.getDouble(0);
    }

    /**
     * Read a null-terminated UTF8 string consisting at most of
     * {@code maxLength} bytes plus the null terminator.
     * 
     * <p>This this method consumes at most {@code maxLength+1} bytes from the
     * stream, a single call consumes only as many bytes as it needs to find
     * the null terminator. If after reading {@code maxLength}
     * non-null bytes the next byte is not the null terminator an
     * {@code EXRIOException} is thrown.</p>
     * 
     * <p>The actual length of the string might be less than {@code maxLength}
     * if it contains multi-byte characters.</p>
     * 
     * @param maxLength the maximum number of non-null bytes expected.
     * @return a decoded UTF string.
     * @throws EXRIOException if the null terminator is missing after consuming
     *         {@code maxLength+1} bytes, or if an I/O error occurs.
     */
    public String readNullTerminatedUTF8(final int maxLength) 
            throws EXRIOException {
        // Just alias the internal buffer if the string is short enough
        final byte[] strBuffer = maxLength <= BUFFER_LEN ?
                buffer.array() : new byte[maxLength];
        assert strBuffer != null;
        
        final int numPasses = (maxLength + BUFFER_LEN - 1) / BUFFER_LEN;
        assert BUFFER_LEN*numPasses >= maxLength;
        int n = 0;
        boolean done = false;
        for (int passIdx = 0; !done && passIdx < numPasses; passIdx++) {
            buffer.rewind();
            while (n <= maxLength && buffer.position() < buffer.capacity()) {
                buffer.limit(buffer.position() + 1);
                is.read(buffer);
                if (buffer.get(buffer.position() - 1) == 0) {
                    done = true;
                    break;
                }
                ++n;
            }
            assert (numPasses == 1 && strBuffer == buffer.array()) ^
                   (numPasses >  1 && strBuffer != buffer.array());
            if (numPasses > 1) {
                final int toCopy = n - passIdx*BUFFER_LEN;
                assert (0 <= toCopy) && (toCopy <= BUFFER_LEN);
                // This should be more efficient than buffer.get(b)
                System.arraycopy(buffer.array(), 0,
                        strBuffer, passIdx*BUFFER_LEN, toCopy);
            }
        }
        
        if (n < 0) {
            throw new EXRIOException("null terminator missing, max length: "
                    + maxLength);
        }
        
        return n > 0 ? new String(strBuffer, 0, n, UTF8) : "";
    }

    /**
     * Read a UTF8 string using {@code length} bytes from the stream.
     * 
     * <p>Even if the string has an earlier null terminator, this method always
     * consumes exactly {@code length} bytes from the stream. 
     * The actual length of the string might be less than {@code maxLength}
     * if it contains multi-byte characters.</p>
     * 
     * @param length number of bytes to read from the stream.
     * @return a decoded UTF8 string.
     * @throws IllegalArgumentException if {@code length < 0}
     * @throws EXRIOException if an I/O error occurs.
     */
    public String readUTF8(int length) throws EXRIOException {
        if (length < 0) {
            throw new IllegalArgumentException("invalid length: " + length);
        } else if (length == 0) {
            return "";
        }
        
        if (length <= BUFFER_LEN) {
            buffer.rewind();
            buffer.limit(length);
            is.read(buffer);
            assert buffer.array() != null;
            return new String(buffer.array(), 0, length, UTF8);
        } else {
            byte[] strBuffer = new byte[length];
            readFully(strBuffer);
            return new String(strBuffer, UTF8);
        }
    }
    
    public void readFully(byte[] b) throws EXRIOException {
        readFully(b, 0, b.length);
    }

    /**
     * 
     * @param b
     * @param off
     * @param len
     * @throws EXRIOException
     * @throws IllegalArgumentException if {@code b} is null.
     * @throws IndexOutOfBoundsException if {@code off} is negative,
     *         {@code len} is negative, or {@code len} is greater than
     *         {@code b.length - off}
     */
    public void readFully(byte[] b, int off, int len) throws EXRIOException {
        if (b == null) {
            throw new IllegalArgumentException("null destination buffer");
        } else if (off < 0) {
            throw new IndexOutOfBoundsException("invalid offset: " + off);
        } else if (len < 0 || len > (b.length - off)) {
            throw new IndexOutOfBoundsException("invalid length: " + len);
        }
        
        final int numPasses = (len + BUFFER_LEN - 1) / BUFFER_LEN;
        assert BUFFER_LEN*numPasses >= len;
        for (int passIdx = 0; passIdx < numPasses; passIdx++) {
            buffer.rewind();
            buffer.limit(Math.min(BUFFER_LEN, len - BUFFER_LEN*passIdx));
            is.read(buffer);
            assert buffer.array() != null;
            // This should be more efficient than buffer.get(b)
            System.arraycopy(buffer.array(), 0,
                    b, BUFFER_LEN*passIdx, buffer.limit());
        }
    }
    
}

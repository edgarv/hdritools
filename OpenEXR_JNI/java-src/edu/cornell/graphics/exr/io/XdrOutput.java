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
import edu.cornell.graphics.exr.Half;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;

/**
 * Helper class which encapsulates an {@code EXROutputStream} to write primitive
 * and string data.
 * 
 * <p>Each instance uses a private buffer to write data into the stream. All
 * methods are <em>not</em> thread safe, and assume that the stream will remain
 * valid through the lifetime of a {@code XdrOutput} instance.</p>
 * 
 * <p>Modeled after the functions in ImfXdr.h from the C++ OpenEXR library.</p>
 * 
 * @see EXROutputStream
 * @since OpenEXR-JNI 3.0
 */
public class XdrOutput {
    
    private final static int BUFFER_LEN = 512;
    
    /** Actual instance of the UTF8 charset to encode/decode strings */
    private final Charset UTF8 = Charset.forName("UTF-8");
    
    private final ByteBuffer buffer;
    private final EXROutputStream os;
    
    public XdrOutput(EXROutputStream stream) {
        if (stream == null) {
            throw new IllegalArgumentException("null input stream");
        }
        buffer = ByteBuffer.allocate(BUFFER_LEN);
        if (!buffer.hasArray()) {
            throw new IllegalStateException("buffer not backed by an array");
        }
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        os = stream;
    }
    
    /**
     * Get the current writing position, in bytes from the beginning of the
     * stream. If the next call to {@code write()} will write the first byte in
     * the stream, {@code position()} returns {@literal 0}.
     * 
     * <p>This implementation simply relies on
     * {@link EXROutputStream#position() }.</p>
     * 
     * @return the current writing position, in bytes from the beginning of the
     *         stream.
     * @throws EXRIOException if an I/O error occurs.
     */
    public long position() throws EXRIOException {
        return os.position();
    }
    
    /**
     * Set the current position. After calling {@code position(i)},
     * {@code position()} returns <em>i</em>.
     * 
     * <p>This implementation simply relies on
     * {@link EXROutputStream#position(long) }.</p>
     * 
     * @param pos the new writing position.
     * @return a reference to this {@code XdrOutput} instance
     */
    public XdrOutput position(long pos) throws EXRIOException {
        os.position(pos);
        return this;
    }
    
    public void writeBoolean(boolean value) throws EXRIOException {
        writeByte(value ? (byte) 1 : 0);
    }
    
    public void writeByte(byte b) throws EXRIOException {
        buffer.rewind().limit(1);
        buffer.put(b);
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeUnsignedByte(int value) throws EXRIOException {
        buffer.rewind().limit(1);
        buffer.put((byte) (value & 0xff));
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeShort(short value) throws EXRIOException {
        buffer.rewind().limit(2);
        buffer.putShort(value);
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeUnsignedShort(int value) throws EXRIOException {
        buffer.rewind().limit(2);
        buffer.putShort((short) (value & 0xffff));
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeInt(int value) throws EXRIOException {
        buffer.rewind().limit(4);
        buffer.putInt(value);
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeUnsignedInt(long value) throws EXRIOException {
        buffer.rewind().limit(4);
        buffer.putInt((int) (value & 0xffffffffL));
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeFloat(float value) throws EXRIOException {
        buffer.rewind().limit(4);
        buffer.putFloat(value);
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeDouble(double value) throws EXRIOException {
        buffer.rewind().limit(8);
        buffer.putDouble(value);
        buffer.flip();
        os.write(buffer);
    }
    
    public void writeHalf(Half value) throws EXRIOException {
        final short s = value.halfToRawShortBits();
        writeShort(s);
    }
    
    public void writeNullTerminatedUTF8(String s) throws EXRIOException {
        writeUTF8Impl(s, Integer.MAX_VALUE, true);
    }
    
    public void writeNullTerminatedUTF8(String s, int maxLen) throws EXRIOException {
        writeUTF8Impl(s, maxLen, true);
    }
    
    public void writeUTF8(String s) throws EXRIOException {
        writeUTF8Impl(s, Integer.MAX_VALUE, false);
    }
    
    public void writeUTF8(String s, int maxLen) throws EXRIOException {
        writeUTF8Impl(s, maxLen, false);
    }
    
    private void writeUTF8Impl(String s, int maxLen, boolean appendNull)
            throws EXRIOException {
        final byte[] b = s.getBytes(UTF8);
        if (b.length > maxLen) {
            throw new EXRIOException(
                    "UTF8 encoded string exceeds maximum length: " + b.length);
        }
        writeByteArray(b);
        if (appendNull) {
            writeByte((byte) 0);
        }
    }
    
    /**
     * Add {@code n} padding bytes. The added bytes are set to zero.
     * If {@code n} is less than one the function returns immediately.
     * 
     * @param n number of padding bytes to add.
     */
    public void pad(int n) throws EXRIOException {
        if (n < 1) {
            return;
        }
        int count = 0;
        buffer.limit(buffer.capacity());
        while (count < n) {
            buffer.rewind();
            final int limit = Math.min(n - count, BUFFER_LEN);
            for (int i = 0; i < limit; ++i, ++count) {
                buffer.put((byte) 0);
            }
            buffer.flip();
            os.write(buffer);
        }
        assert count == n;
    }
    
    public void writeByteArray(byte[] b) throws EXRIOException {
        writeByteArray(b, 0,  b.length);
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
    public void writeByteArray(byte[] b,int off,int len) throws EXRIOException {
        if (b == null) {
            throw new IllegalArgumentException("null destination buffer");
        } else if (off < 0) {
            throw new IndexOutOfBoundsException("invalid offset: " + off);
        } else if (len < 0 || len > (b.length - off)) {
            throw new IndexOutOfBoundsException("invalid length: " + len);
        }
        
        final int numPasses = (len + BUFFER_LEN - 1) / BUFFER_LEN;
        assert BUFFER_LEN*numPasses >= len;
        buffer.limit(buffer.capacity());
        for (int passIdx = 0; passIdx < numPasses; ++passIdx) {
            final int length = Math.min(BUFFER_LEN, len - BUFFER_LEN*passIdx);
            final int offset = off + BUFFER_LEN*passIdx;
            buffer.rewind();
            buffer.put(b, offset, length).flip();
            os.write(buffer);
        }
    }
    
    public void writeBuffer(ByteBuffer src) throws EXRIOException {
        os.write(src);
    }
    
}

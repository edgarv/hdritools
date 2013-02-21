/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

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

import java.io.DataInput;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.Charset;

/**
 * Buffered data input whose main use is to read data from OpenEXR files,
 * always in little-endian format, for further processing and decoding.
 * This is a low-level IO class which the final users should not need to use.
 */
public class EXRBufferedDataInput implements DataInput {
    
    /** Helper transfer object to know the actual amount of bytes read. */
    public static class BytesReadTO {
        public int count;
    }
    
    /** Size of the input buffer in bytes */
    private final static int BUFFER_SIZE = 8192;
    
    /** Actual instance of the UTF8 charset to encode/decode strings */
    private final Charset UTF8 = Charset.forName("UTF-8");
    
    /** Reference to the actual input stream */
    private final InputStream input;
    
    /** Internal buffer */
    private final byte[] buffer = new byte[BUFFER_SIZE];
    
    /** Currently available bytes in the buffer */
    private int bufLen = 0;
    
    /** Current position within the buffer */
    private int bufPos = 0;
    
    public EXRBufferedDataInput(InputStream input) {
        if (input == null) {
            throw new IllegalArgumentException("Null input stream.");
        }
        this.input = input;
    }
    
    /**
     * Returns the number of bytes that can be read (or skipped over) without
     * blocking by the next invocation of a method for this data input. This
     * method returns the bytes remaining in the internal buffer
     * 
     * @return the number of bytes that can be read (or skipped over) without
     *         blocking
     */
    public int available() {
        return bufLen - bufPos;
    }
    
    /**
     * Guarantees that the internal buffer contains at least {@code numBytes}
     * bytes ready to be immediately read.
     * Throws {@code IllegalArgumentException} if {@code numBytes} is less than
     * one or greater than the internal buffer length.
     * 
     * @param numBytes the number of bytes desired.
     * @throws IOException if an I/O error occurs.
     */
    private void requireAvailableBytes(int numBytes) throws IOException {
        if (numBytes < 1 || numBytes > buffer.length) {
            throw new IllegalArgumentException("Invalid request");
        }

        final int origAvailable = available();
        if (origAvailable < numBytes) {
            // Move the currently avaialable bytes to the start of the buffer.
            // Note that System.arraycopy handles overlapping regions properly
            System.arraycopy(buffer, bufPos, buffer, 0, origAvailable);
            bufLen = origAvailable;
            bufPos = 0;
            
            // Fill the buffer until it has enough data
            while (available() < numBytes) {
                int nBytes = input.read(buffer, bufPos, buffer.length - bufLen);
                if (nBytes < 0) {
                    throw new EOFException();
                }
                bufLen += nBytes;
            }
        }
        if (available() < numBytes) {
            throw new IllegalStateException("Failed to buffer enough data.");
        }
    }
    
    public byte peekByte() throws IOException {
        requireAvailableBytes(1);
        return buffer[bufPos];
    }
    
    /**
     * Reads one input byte, zero-extends it to type {@code int}, and returns
     * the result, which is therefore in the range {@code 0} through
     * {@code 255}. This method is suitable for reading the byte written by the
     * {@code writeByte} method of interface {@code DataOutput} if the argument
     * to {@code writeByte} was intended to be a value in the range {@code 0}
     * through {@code 255}.
     * 
     * @return the unsigned 8-bit value read.
     * @throws EOFException if this stream reaches the end before reading all
     *                      the bytes.
     * @throws IOException if an I/O error occurs.
     */
    @Override
    public byte readByte() throws EOFException, IOException {
        requireAvailableBytes(1);
        return buffer[bufPos++];
    }

    @Override
    public void readFully(byte[] b) throws IOException {
        readFully(b, 0, b.length);
    }

    @Override
    public void readFully(byte[] b, int off, int len) throws IOException {
        int curOff = off;
        int needed = len;
        // Copy data already in the buffer
        if (available() != 0) {
            final int origAvailable = Math.min(len, available());
            System.arraycopy(buffer, bufPos, b, curOff, origAvailable);
            bufPos += origAvailable;
            curOff += origAvailable;
            needed -= origAvailable;
        }
        assert needed >= 0;
        
        // Read the rest of the data directly without modifying the buffer
        while (needed != 0) {
            assert bufPos >= bufLen;
            int bytesRead = input.read(b, curOff, needed);
            if (bytesRead < 0) {
                throw new EOFException();
            }
            curOff += bytesRead;
            needed -= bytesRead;
            assert needed >= 0;
        }
    }

    @Override
    public int skipBytes(int n) throws IOException {
        if (n <= 0) {
            return 0;
        }
        // Discard buffer data
        int totalCount = Math.min(n, available());
        bufPos += totalCount;
        assert bufPos <= bufLen;
        if (totalCount == n) {
            return totalCount;
        }
        // Relly on the underlying stream
        assert n > totalCount;
        totalCount += (int) input.skip(n - totalCount);
        return totalCount;
    }

    @Override
    public boolean readBoolean() throws IOException {
        return readByte() != 0;
    }

    @Override
    public int readUnsignedByte() throws IOException {
        return readByte() & 0xff;
    }

    /**
     * Reads a {@code short} as specified by {@link DataInput#readShort()},
     * except using little-endian byte order.
     * 
     * @return the next two bytes of the input stream, interpreted as a
     *         {@code short} in little-endian byte order.
     * @throws IOException if an I/O error occurs.
     */
    @Override
    public short readShort() throws IOException {
        requireAvailableBytes(2);
        int a = buffer[bufPos++] & 0xff;
        int b = buffer[bufPos++] & 0xff;
        short s = (short) ((b << 8) | a);
        return s;
    }

    @Override
    public int readUnsignedShort() throws IOException {
        requireAvailableBytes(2);
        int a = buffer[bufPos++] & 0xff;
        int b = buffer[bufPos++] & 0xff;
        int s = (b << 8) | a;
        return s;
    }

    @Override
    public char readChar() throws IOException {
        requireAvailableBytes(2);
        int a = buffer[bufPos++] & 0xff;
        int b = buffer[bufPos++] & 0xff;
        char c = (char) ((b << 8) | a);
        return c;
    }

    @Override
    public int readInt() throws IOException {
        requireAvailableBytes(4);
        int a = buffer[bufPos++] & 0xff;
        int b = buffer[bufPos++] & 0xff;
        int c = buffer[bufPos++] & 0xff;
        int d = buffer[bufPos++] & 0xff;
        int n = (d << 24) | (c << 16) | (b << 8) | a;
        return n;
    }

    @Override
    public long readLong() throws IOException {
        requireAvailableBytes(8);
        long a = buffer[bufPos++] & 0xff;
        long b = buffer[bufPos++] & 0xff;
        long c = buffer[bufPos++] & 0xff;
        long d = buffer[bufPos++] & 0xff;
        long e = buffer[bufPos++] & 0xff;
        long f = buffer[bufPos++] & 0xff;
        long g = buffer[bufPos++] & 0xff;
        long h = buffer[bufPos++] & 0xff;
        long n = (h << 56) | (g << 48) | (f << 40) | (e << 32) |
                 (d << 24) | (c << 16) | (b <<  8) |  a;
        return n;
    }

    @Override
    public float readFloat() throws IOException {
        final int bits = readInt();
        return Float.intBitsToFloat(bits);
    }

    @Override
    public double readDouble() throws IOException {
        final long bits = readLong();
        return Double.longBitsToDouble(bits);
    }

    @Override
    public String readLine() throws IOException {
        throw new UnsupportedOperationException("Not supported.");
    }

    @Override
    public String readUTF() throws IOException {
        throw new UnsupportedOperationException("Not supported.");
    }
    
    public String readNullTerminatedUTF8(int maxLength) throws IOException {
        return readNullTerminatedUTF8(maxLength, null);
    }

    public String readNullTerminatedUTF8(int maxLength, BytesReadTO bytesRead)
            throws IOException {
        if (maxLength < 1) {
            throw new IllegalArgumentException("Invalid lenght: " + maxLength);
        }
        if (maxLength >= buffer.length) {
            throw new IllegalArgumentException("The requested maximum length " +
                    "exceeds the internal buffer capacity (" + maxLength + ")");
        }
        // The +1 is because a string of length "maxLength" still need the
        // null character terminator
        for (int i = 0; i < maxLength+1; ++i) {
            // We need up to i+i characters avaiable
            requireAvailableBytes(i + 1);
            if (buffer[bufPos + i] == 0) {
                // Build a UTF8 string from the current byte sequence, excluding
                // the trailing null character
                String s = new String(buffer, bufPos, i, UTF8);
                bufPos += i + 1;
                if (bytesRead != null) {
                    bytesRead.count = i + 1;
                }
                return s;
            }
        }
        throw new IOException("Invalid string: null terminator not found " +
                "after reading " + (maxLength+1) + "bytes.");
    }
    
    public String readNullTerminatedUTF8() throws IOException {
        return readNullTerminatedUTF8(buffer.length - 1);
    }
    
    public String readUTF8(int length) throws IOException {
        if (length < 0) {
            throw new IllegalArgumentException("Invalid lenght: " + length);
        }
        if (length <= buffer.length) {
            // Simple case: the string fits in the buffer
            requireAvailableBytes(length);
            String s = new String(buffer, bufPos, length, UTF8);
            bufPos += length;
            return s;
        } else {
            // More complicated case: get as many characters as possible from
            // the buffer, the rest directly from the data input
            byte[] utf8Buffer = new byte[length];
            int count = available();
            readFully(utf8Buffer, 0, count);
            
            while (count < length) {
                int nBytes = input.read(utf8Buffer, count, length - count);
                if (nBytes < 0) {
                    throw new EOFException();
                }
                count += nBytes;
            }
            
            String s = new String(utf8Buffer, UTF8);
            return s;
        }
    }
}

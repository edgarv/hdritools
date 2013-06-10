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
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.util.Objects;

/**
 * An EXR input stream relying on an existing {@link SeekableByteChannel}.
 * 
 * <p>This class assumes that the {@code SeekableByteChannel} specified during
 * its construction will remain open through the lifetime of an instance.</p>
 * 
 * @since OpenEXR-JNI 3.0
 */
public class ByteChannelInputStream implements EXRInputStream {
    
    private final SeekableByteChannel channel;
    
    /**
     * Constructs a {@code ByteChannelInputStream} which relies on a non-null,
     * already open {@code SeekableByteChannel}.
     * 
     * @param channel the {@code SeekableByteChannel} to be used by the
     *        newly constructed instance
     * @throws NullPointerException is {@code channel} is null
     * @throws IllegalArgumentException is {@code channel} is not open
     */
    public ByteChannelInputStream(SeekableByteChannel channel) {
        this.channel = Objects.requireNonNull(channel);
        if (!channel.isOpen()) {
            throw new IllegalArgumentException("channel is not open");
        }
    }

    @Override
    public boolean read(ByteBuffer dst) throws EXRIOException {
        final int length = dst.remaining();
        if (length < 0) {
            throw new IllegalArgumentException("Invalid requested bytes");
        }
        
        int count = 0;
        try {
            while (count < length) {
                int nBytes = channel.read(dst);
                if (nBytes < 0) {
                    throw new EXRIOException("Premature EOF");
                }
                count += nBytes;
            }
            if (count > length) {
                throw new IllegalStateException("Read excess data!");
            }
            final boolean hasMoreData = channel.position() < channel.size();
            return hasMoreData;
        } catch (EXRIOException ex1) {
            throw ex1;
        } catch (IOException ex2) {
            throw new EXRIOException(ex2.getMessage(), ex2);
        }
    }

    @Override
    public long position() throws EXRIOException {
        try {
            return channel.position();
        } catch (IOException ex) {
            throw new EXRIOException(ex.getMessage(), ex);
        }
    }

    @Override
    public void position(long pos) throws EXRIOException {
        try {
            channel.position(pos);
        } catch (IOException ex) {
            throw new EXRIOException(ex.getMessage(), ex);
        }
    }
    
}

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

// TODO Add documentation
public class ByteChannelOutputStream implements EXROutputStream {
    
    private final SeekableByteChannel channel;

    public ByteChannelOutputStream(SeekableByteChannel channel) {
        if (channel == null) {
            throw new IllegalArgumentException("null channel");
        }
        this.channel = channel;
    }

    @Override
    public void write(ByteBuffer src) throws EXRIOException {
        final int length = src.remaining();
        if (length < 0) {
            throw new IllegalArgumentException("Invalid requested bytes");
        }
        
        int count = 0;
        try {
            while (count < length) {
                int nBytes = channel.write(src);
                if (nBytes < 0) {
                    throw new EXRIOException("Unexpected state after writing");
                }
                count += nBytes;
            }
            if (count > length) {
                throw new IllegalStateException("Wrote excess data!");
            }
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
    public EXROutputStream position(long pos) throws EXRIOException {
        try {
            channel.position(pos);
            return this;
        } catch (IOException ex) {
            throw new EXRIOException(ex.getMessage(), ex);
        }
    }
    
}

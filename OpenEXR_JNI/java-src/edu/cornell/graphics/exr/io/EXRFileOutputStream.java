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
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;

/**
 * Helper class which simplifies creating an {@code EXROutputStream} from a
 * file. This class acquires an exclusive write lock on the file and implements
 * {@code AutoCloseable} so that it may be used in {@code try}-with-resources.
 */
public class EXRFileOutputStream implements EXROutputStream, AutoCloseable {
    
    private final FileChannel channel;
    private final FileLock lock;
    private final ByteChannelOutputStream os;
    
    public EXRFileOutputStream(Path path) throws IOException {
        if (path == null) {
            throw new IllegalArgumentException("null path");
        }
        channel = FileChannel.open(path, StandardOpenOption.WRITE,
                StandardOpenOption.TRUNCATE_EXISTING);
        lock = channel.tryLock(0, Long.MAX_VALUE, false);
        if (lock == null) {
            throw new EXRIOException("Could not get an exclusive lock on " + path);
        }
        os = new ByteChannelOutputStream(channel);
    }
    
    @Override
    public void close() throws IOException {
        if (channel.isOpen()) {
            lock.release();
            channel.close();
        }
    }

    @Override
    public void write(ByteBuffer src) throws EXRIOException {
        os.write(src);
    }

    @Override
    public long position() throws EXRIOException {
        return os.position();
    }

    @Override
    public EXROutputStream position(long pos) throws EXRIOException {
        os.position(pos);
        return this;
    }
    
}

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
 * Helper class which simplifies creating an {@code EXRInputStream} from a
 * file. This class acquires a shared lock on the file and implements
 * {@code AutoCloseable} so that it may be used in {@code try}-with-resources.
 * 
 * @since OpenEXR-JNI 3.0
 */
public class EXRFileInputStream implements EXRInputStream, AutoCloseable {
    
    private final FileChannel channel;
    private final FileLock lock;
    private final ByteChannelInputStream is;
    
    public EXRFileInputStream(Path path) throws EXRIOException {
        if (path == null) {
            throw new IllegalArgumentException("null path");
        }
        try {
            channel = FileChannel.open(path, StandardOpenOption.READ);
            lock = channel.tryLock(0, Long.MAX_VALUE, true);
            if (lock == null) {
                throw new EXRIOException("Could not get a shared lock on " + path);
            }
            is = new ByteChannelInputStream(channel);
        } catch (EXRIOException ex1) {
            throw ex1;
        } catch (IOException ex2) {
            throw new EXRIOException(ex2.getMessage(), ex2);
        }
    }

    @Override
    public void close() throws EXRIOException {
        if (channel.isOpen()) {
            try {
                lock.release();
                channel.close();
            } catch (IOException ex) {
                throw new EXRIOException(ex.getMessage(), ex);
            }
        }
    }

    @Override
    public boolean read(ByteBuffer dst) throws EXRIOException {
        return is.read(dst);
    }

    @Override
    public long position() throws EXRIOException {
        return is.position();
    }

    @Override
    public void position(long pos) throws EXRIOException {
        is.position(pos);
    }
    
}

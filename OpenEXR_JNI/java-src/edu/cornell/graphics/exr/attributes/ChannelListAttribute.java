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

package edu.cornell.graphics.exr.attributes;

import edu.cornell.graphics.exr.Channel;
import edu.cornell.graphics.exr.ChannelList;
import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.EXRVersion;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput.BytesReadTO;
import java.io.IOException;

// TODO: Add documentation
public class ChannelListAttribute extends TypedAttribute<ChannelList> {

    public ChannelListAttribute() {}
    
    public ChannelListAttribute(ChannelList chlist) {
        setValue(chlist);
    }

    @Override
    public String typeName() {
        return "chlist";
    }
    
    private static void readChannel(EXRBufferedDataInput input, ChannelList lst,
            int maxNameLength, BytesReadTO bytesRead) throws
            EXRIOException, IOException {
        assert bytesRead != null;
        String name = input.readNullTerminatedUTF8(maxNameLength, bytesRead);
        if (name.isEmpty()) {
            throw new EXRIOException("Empty channel name.");
        }

        final Channel c = new Channel();
        int typeOrdinal = input.readInt();
        c.type = checkedValueOf(typeOrdinal, Channel.PixelType.values());

        c.pLinear = input.readBoolean();
        byte[] reserved = new byte[3];
        input.readFully(reserved);
        if (((reserved[2] << 16) | (reserved[1] << 8) | reserved[0]) != 0) {
            throw new EXRIOException("Reserved bytes have non-zero values");
        }
        c.xSampling = input.readInt();
        c.ySampling = input.readInt();
        
        lst.insert(name, c);
        
        // Add the fixed length fields
        bytesRead.count += 4 + 1 + 3 + 4 + 4;
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
        throws EXRIOException, IOException {
        
        ChannelList chlist = getValue();
        if (chlist == null) {
            chlist = new ChannelList();
            setValue(chlist);
        } else {
            chlist.clear();
        }
        
        int readCount = 0;
        BytesReadTO bytesRead = new BytesReadTO();
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        while (input.peekByte() != 0) {
            readChannel(input, chlist, maxNameLength, bytesRead);
            readCount += bytesRead.count;
        }
        // Consume the last byte
        if (input.readByte() != 0) {
            throw new IllegalStateException("Missing trailing 0x0");
        }
        checkSize(++readCount, size);
    }
}

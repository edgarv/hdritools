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
            int maxNameLength) throws EXRIOException, IOException {
        final long origBytesUsed = input.getBytesUsed();
        String name = input.readNullTerminatedUTF8(maxNameLength);
        final long nameBytes = input.getBytesUsed() - origBytesUsed;
        assert nameBytes > 0;
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
        assert (nameBytes + (4 + 1 + 3 + 4 + 4)) ==
               (input.getBytesUsed() - origBytesUsed);
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
        
        final long origBytesUsed = input.getBytesUsed();
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        while (input.peekByte() != 0) {
            readChannel(input, chlist, maxNameLength);
        }
        // Consume the last byte
        if (input.readByte() != 0) {
            throw new IllegalStateException("Missing trailing 0x0");
        }
        final int readCount = (int) (input.getBytesUsed() - origBytesUsed);
        checkSize(readCount, size);
    }

    @Override
    protected ChannelList cloneValue() {
        ChannelList chlist = new ChannelList();
        for (ChannelList.ChannelListElement e : value) {
            final Channel orig = e.getChannel();
            Channel channel   = new Channel();
            channel.pLinear   = orig.pLinear;
            channel.type      = orig.type;
            channel.xSampling = orig.xSampling;
            channel.ySampling = orig.ySampling;
            chlist.insert(e.getName(), channel);
        }
        return chlist;
    }
    
}

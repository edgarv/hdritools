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
import edu.cornell.graphics.exr.ChannelList.ChannelListElement;
import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.EXRVersion;
import edu.cornell.graphics.exr.PixelType;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;
import java.io.IOException;

// TODO: Add documentation
public class ChannelListAttribute extends TypedAttribute<ChannelList> {
    
    private static class Pair {
        final String name;
        final Channel c;

        private Pair(String name) {
            this.name = name;
            this.c = new Channel();
        }
    }

    public ChannelListAttribute() {}
    
    public ChannelListAttribute(ChannelList chlist) {
        setValue(chlist);
    }

    @Override
    public String typeName() {
        return "chlist";
    }
    
    private static Pair readChannel(XdrInput input, int maxNameLength) 
            throws EXRIOException, IOException {
        final long p0 = input.position();
        String name = input.readNullTerminatedUTF8(maxNameLength);
        final long nameBytes = input.position() - p0;
        if (name.isEmpty()) {
            return null;
        }

        final Pair pair = new Pair(name);
        int typeOrdinal = input.readInt();
        pair.c.type = checkedValueOf(typeOrdinal, PixelType.values());

        pair.c.pLinear = input.readBoolean();
        byte[] reserved = new byte[3];
        input.readFully(reserved);
        if (((reserved[2] << 16) | (reserved[1] << 8) | reserved[0]) != 0) {
            throw new EXRIOException("Reserved bytes have non-zero values");
        }
        pair.c.xSampling = input.readInt();
        pair.c.ySampling = input.readInt();
        
        // Add the fixed length fields
        assert (nameBytes + (4 + 1 + 3 + 4 + 4)) == (input.position() - p0);
        return pair;
    }

    @Override
    protected void readValueFrom(XdrInput input, int version)
        throws EXRIOException, IOException {
        
        ChannelList chlist = getValue();
        if (chlist == null) {
            chlist = new ChannelList();
            setValue(chlist);
        } else {
            chlist.clear();
        }
        
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        Pair pair;
        while ((pair = readChannel(input, maxNameLength)) != null) {
            chlist.insert(pair.name, pair.c);
        }
    }

    @Override
    public void writeValueTo(XdrOutput output, int version)
            throws EXRIOException {
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        final ChannelList chlist = getValue();
        
        for (ChannelListElement c : chlist) {
            // Write name
            output.writeNullTerminatedUTF8(c.getName(), maxNameLength);
            
            // Write channel struct
            final Channel channel = c.getChannel();
            output.writeInt(channel.type.ordinal());
            output.writeBoolean(channel.pLinear);
            output.pad(3);
            output.writeInt(channel.xSampling);
            output.writeInt(channel.ySampling);
        }
        
        // Write end of list marker
        output.writeNullTerminatedUTF8("");
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

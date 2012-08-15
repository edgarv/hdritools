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
import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.EXRVersion;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput.BytesReadTO;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

// TODO: Add documentation
public class ChannelListAttribute extends TypedAttribute<List<Channel>> {

    public ChannelListAttribute() {}
    
    public ChannelListAttribute(ArrayList<Channel> list) {
        setValue(list);
    }

    @Override
    public String typeName() {
        return "chlist";
    }
    
    private static Channel readChannel(EXRBufferedDataInput input,
            int maxNameLength, BytesReadTO bytesRead) throws
            EXRIOException, IOException {
        assert bytesRead != null;
        Channel c = new Channel();
        c.name = input.readNullTerminatedUTF8(maxNameLength, bytesRead);
        if (c.name.isEmpty()) {
            throw new EXRIOException("Empty channel name.");
        }

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
        
        // Add the fixed length fields
        bytesRead.count += 4 + 1 + 3 + 4 + 4;

        return c;
    }

    @Override
    public void readValueFrom(EXRBufferedDataInput input, int size, int version)
        throws EXRIOException, IOException {
        
        List<Channel> chlist = getValue();
        if (chlist == null) {
            chlist = new ArrayList<Channel>();
            setValue(chlist);
        } else {
            chlist.clear();
        }
        
        int readCount = 0;
        BytesReadTO bytesRead = new BytesReadTO();
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        while (input.peekByte() != 0) {
            Channel c = readChannel(input, maxNameLength, bytesRead);
            readCount += bytesRead.count;
            chlist.add(c);
        }
        // Consume the last byte
        if (input.readByte() != 0) {
            throw new IllegalStateException("Missing trailing 0x0");
        }
        checkSize(++readCount, size);
    }
}

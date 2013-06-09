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

import edu.cornell.graphics.exr.EXRIOException;
import edu.cornell.graphics.exr.EXRVersion;
import edu.cornell.graphics.exr.Header;
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * A simpler version of a full input file: this class only reads the header
 * to get information about the file, closing the input once the data has been
 * read.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class InputFileInfo {
    
    private int version;
    
    private String filename;
    
    private Boolean completeScanLine;
    
    private final Header header = new Header();
    
    private void initialize(XdrInput input) throws EXRIOException {
        if (input == null) {
            throw new IllegalArgumentException("Null data input.");
        }
        
        int magic = input.readInt();
        if (magic != EXRVersion.MAGIC) {
            throw new EXRIOException("Invalid magic number");
        }
        version = input.readInt();
        if (EXRVersion.getVersion(version) != EXRVersion.EXR_VERSION ||
            !EXRVersion.supportsFlags(EXRVersion.getFlags(version))) {
            throw new EXRIOException("File contains unsupported features: " +
                    Integer.toHexString(version));
        }
        
        header.readFrom(input, version);
        if (!isTiled()) {
            completeScanLine = checkLineOffsets(header, input);
        }
    }
    
    private static boolean checkLineOffsets(Header hdr, XdrInput input) {
        try {
            final Box2<Integer> dw  = hdr.getDataWindow();
            final int linesInBuffer = hdr.getCompression().numScanLines();
            final int numLineOffsets =
                    (dw.yMax - dw.yMin + linesInBuffer) / linesInBuffer;
            for (int i = 0; i < numLineOffsets; ++i) {
                long offset = input.readLong();
                if (offset <= 0) {
                    return false;
                }
            }
            return true;
        } catch (Exception ex) {
            return false;
        }
    }
    
    public InputFileInfo(XdrInput input) throws EXRIOException {
        initialize(input);
    }
    
    public InputFileInfo(String filename) throws EXRIOException {
        this(Paths.get(filename));
    }
    
    public InputFileInfo(File file) throws EXRIOException {
        this(file.toPath());
    }

    public InputFileInfo(Path path) throws EXRIOException {
        try (EXRFileInputStream is = new EXRFileInputStream(path)) {
            XdrInput input = new XdrInput(is);
            initialize(input);
            filename = path.toAbsolutePath().toString();
        }
    }
    
    public boolean isTiled() {
        return EXRVersion.isTiled(version);
    }
    
    public int getVersion() {
        return version;
    }
    
    public Header getHeader() {
        return header;
    }
    
    public String getFilename() {
        return filename;
    }

    public boolean isComplete() {
        if (!isTiled()) {
            return completeScanLine;
        } else {
            throw new UnsupportedOperationException("Not implemented yet.");
        }
    }
    
}

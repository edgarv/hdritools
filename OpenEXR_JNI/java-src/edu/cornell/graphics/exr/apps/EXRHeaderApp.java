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

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

package edu.cornell.graphics.exr.apps;

import edu.cornell.graphics.exr.Channel;
import edu.cornell.graphics.exr.ChannelList;
import edu.cornell.graphics.exr.Chromaticities;
import edu.cornell.graphics.exr.Compression;
import edu.cornell.graphics.exr.EXRVersion;
import edu.cornell.graphics.exr.EnvMap;
import edu.cornell.graphics.exr.Header;
import edu.cornell.graphics.exr.KeyCode;
import edu.cornell.graphics.exr.LineOrder;
import edu.cornell.graphics.exr.PixelType;
import edu.cornell.graphics.exr.PreviewImage;
import edu.cornell.graphics.exr.Rational;
import edu.cornell.graphics.exr.TileDescription;
import edu.cornell.graphics.exr.TimeCode;
import edu.cornell.graphics.exr.attributes.Attribute;
import edu.cornell.graphics.exr.attributes.ChannelListAttribute;
import edu.cornell.graphics.exr.attributes.ChromaticitiesAttribute;
import edu.cornell.graphics.exr.attributes.CompressionAttribute;
import edu.cornell.graphics.exr.attributes.EnvMapAttribute;
import edu.cornell.graphics.exr.attributes.KeyCodeAttribute;
import edu.cornell.graphics.exr.attributes.LineOrderAttribute;
import edu.cornell.graphics.exr.attributes.M33dAttribute;
import edu.cornell.graphics.exr.attributes.M33fAttribute;
import edu.cornell.graphics.exr.attributes.M44dAttribute;
import edu.cornell.graphics.exr.attributes.M44fAttribute;
import edu.cornell.graphics.exr.attributes.OpaqueAttribute;
import edu.cornell.graphics.exr.attributes.PreviewImageAttribute;
import edu.cornell.graphics.exr.attributes.RationalAttribute;
import edu.cornell.graphics.exr.attributes.StringAttribute;
import edu.cornell.graphics.exr.attributes.StringVectorAttribute;
import edu.cornell.graphics.exr.attributes.TileDescriptionAttribute;
import edu.cornell.graphics.exr.attributes.TimeCodeAttribute;
import edu.cornell.graphics.exr.attributes.TypedAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Matrix33;
import edu.cornell.graphics.exr.ilmbaseto.Matrix44;
import edu.cornell.graphics.exr.io.InputFileInfo;
import java.io.File;
import java.io.IOException;
import java.io.PrintStream;
import java.util.List;
import java.util.Map.Entry;

/**
 * Utility program to print an image file's header. Based on the original
 * {@code exrheader} program included in the standard OpenEXR distribution.
 * 
 * @since OpenEXR-JNI 2.1
 */
public class EXRHeaderApp {
    
    private static String toString(Compression c) {
        switch(c) {
            case NONE:
                return "none";
            case RLE:
                return "run-length encoding";
            case ZIPS:
                return "zip, individual scanlines";
            case ZIP:
                return "zip, multi-scanline blocks";
            case PIZ:
                return "piz";
            case PXR24:
                return "pxr24";
            case B44:
                return "b44";
            case B44A:
                return "b44a";
            default:
                return c.toString();
        }
    }
    
    private static String toString(LineOrder lo) {
        switch(lo) {
            case INCREASING_Y:
                return "increasing y";
            case DECREASING_Y:
                return "decreasing y";
            case RANDOM_Y:
                return "random y";
            default:
                return lo.toString();
        }
    }
    
    private static String toString(PixelType pt) {
        switch(pt) {
            case UINT:
                return "32-bit unsigned integer";
            case HALF:
                return "16-bit floating-point";
            case FLOAT:
                return "32-bit floating-point";
            default:
                return "type " + pt.toString();
        }
    }
    
    private static String toString(TileDescription.LevelMode lm) {
        switch(lm) {
            case ONE_LEVEL:
                return "single level";
            case MIPMAP_LEVELS:
                return "mip-map";
            case RIPMAP_LEVELS:
                return "rip-map";
            default:
                return "level mode " + lm.toString();
        }
    }
    
    private static String toString(TileDescription.RoundingMode rm) {
        switch(rm) {
            case ROUND_DOWN:
                return "down";
            case ROUND_UP:
                return "up";
            default:
                return "mode " + rm.toString();
        }
    }
    
    private static String toString(TimeCode tc) {
        StringBuilder sb = new StringBuilder(160);
        sb.append("    time ");
        sb.append(String.format("%02d:%02d:%02d:%02d%n",
            tc.getHours(), tc.getMinutes(), tc.getSeconds(), tc.getFrame()));
        sb.append("    ");
        sb.append(String.format("drop frame: %b, ",  tc.hasDropFrame()));
        sb.append(String.format("color frame: %b, ", tc.hasColorFrame()));
        sb.append(String.format("field/phase: %b%n", tc.hasFieldPhase()));
        sb.append("    ");
        sb.append(String.format("bgf0: %b, ", tc.hasBgf0()));
        sb.append(String.format("bgf1: %b, ", tc.hasBgf1()));
        sb.append(String.format("bgf2: %b%n", tc.hasBgf2()));
        sb.append("    ");
        sb.append(String.format("user data: %#x", tc.getUserData()));
        return sb.toString();
    }
    
    private static String toString(EnvMap e) {
        switch(e) {
            case LATLONG:
                return "latitude-longitude map";
            case CUBE:
                return "cube-face map";
            default:
                return "map type " + e.toString();
        }
    }
    
    private static String toString(ChannelList cl) {
        StringBuilder sb = new StringBuilder(128);
        for (ChannelList.ChannelListElement elem : cl) {
            final Channel channel = elem.getChannel();
            sb.append("\n    ");
            sb.append(elem.getName());
            sb.append(", ");
            sb.append(toString(channel.type));
            sb.append(", sampling ");
            sb.append(channel.xSampling);
            sb.append(' ');
            sb.append(channel.ySampling);
            if (channel.pLinear) {
                sb.append(", plinear");
            }
        }
        return sb.toString();
    }
    
    private static String toString(Matrix33<? extends Number> m) {
        StringBuilder sb = new StringBuilder(64);
        sb.append(":\n   (");
        sb.append(m.m00); sb.append(' ');
        sb.append(m.m01); sb.append(' ');
        sb.append(m.m02); sb.append("\n    ");
        sb.append(m.m10); sb.append(' ');
        sb.append(m.m11); sb.append(' ');
        sb.append(m.m12); sb.append("\n    ");
        sb.append(m.m20); sb.append(' ');
        sb.append(m.m21); sb.append(' ');
        sb.append(m.m22); sb.append(')');
        return sb.toString();
    }
    
    private static String toString(Matrix44<? extends Number> m) {
        StringBuilder sb = new StringBuilder(128);
        sb.append(":\n   (");
        sb.append(m.m00); sb.append(' ');
        sb.append(m.m01); sb.append(' ');
        sb.append(m.m02); sb.append(' ');
        sb.append(m.m03); sb.append("\n    ");
        sb.append(m.m10); sb.append(' ');
        sb.append(m.m11); sb.append(' ');
        sb.append(m.m12); sb.append(' ');
        sb.append(m.m13); sb.append("\n    ");
        sb.append(m.m20); sb.append(' ');
        sb.append(m.m21); sb.append(' ');
        sb.append(m.m22); sb.append(' ');
        sb.append(m.m23); sb.append("\n    ");
        sb.append(m.m30); sb.append(' ');
        sb.append(m.m31); sb.append(' ');
        sb.append(m.m32); sb.append(' ');
        sb.append(m.m33); sb.append(')');
        return sb.toString();
    }
    
    private static String toString(KeyCode k) {
        StringBuilder sb = new StringBuilder(128);
        sb.append(":\n");
        sb.append("    film manufacturer code ");
        sb.append(k.filmMfcCode);
        sb.append('\n');
        sb.append("    film type code ");
        sb.append(k.filmType);
        sb.append('\n');
        sb.append("    prefix ");
        sb.append(k.prefix);
        sb.append('\n');
        sb.append("    count ");
        sb.append(k.count);
        sb.append('\n');
        sb.append("    perf offset ");
        sb.append(k.perfOffset);
        sb.append('\n');
        sb.append("    perfs per frame ");
        sb.append(k.perfsPerFrame);
        sb.append('\n');
        sb.append("    perfs per count ");
        sb.append(k.perfsPerCount);
        return sb.toString();
    }
    
    
    public static void printInfo(PrintStream out, File file) throws IOException{
        final InputFileInfo info = new InputFileInfo(file);
        final Header header = info.getHeader();
        
        out.printf("%n%s%s:%n%n", file.getAbsolutePath(),
                info.isComplete() ? "" : " (incomplete file)");
        
        out.printf("file format version: %d, flags %#x%n",
                EXRVersion.getVersion(info.getVersion()),
                EXRVersion.getFlags(info.getVersion()));
        
        // Iterate over all attributes
        for (Entry<String, Attribute> entry : header) {
            Attribute a = entry.getValue();
            out.printf("%s (type %s)", entry.getKey(), a.typeName());
            
            if (a instanceof ChannelListAttribute) {
                ChannelList chlist = ((ChannelListAttribute)a).getValue();
                if (chlist.size() > 1) {
                    out.printf(" [x %d] ", chlist.size());
                }
                out.print(':');
                out.print(toString(chlist));
            }
            else if (a instanceof ChromaticitiesAttribute) {
                Chromaticities c = ((ChromaticitiesAttribute)a).getValue();
                out.printf(":%n");
                out.printf("    red   (%g - %g)%n", c.redX,   c.redY);
                out.printf("    green (%g - %g)%n", c.greenX, c.greenY);
                out.printf("    blue  (%g - %g)%n", c.blueX,  c.blueY);
                out.printf("    white (%g - %g)%n", c.whiteX, c.whiteY);
            }
            else if (a instanceof CompressionAttribute) {
                Compression c = ((CompressionAttribute)a).getValue();
                out.printf(": %s", toString(c));
            }
            else if (a instanceof EnvMapAttribute) {
                EnvMap envmap = ((EnvMapAttribute)a).getValue();
                out.printf(": %s", toString(envmap));
            }
            else if (a instanceof KeyCodeAttribute) {
                KeyCode keyCode = ((KeyCodeAttribute)a).getValue();
                out.print(toString(keyCode));
            }
            else if (a instanceof LineOrderAttribute) {
                LineOrder lineOrder = ((LineOrderAttribute)a).getValue();
                out.printf(": %s", toString(lineOrder));
            }
            else if (a instanceof M33fAttribute) {
                Matrix33<Float> m = ((M33fAttribute)a).getValue();
                out.print(toString(m));
            }
            else if (a instanceof M33dAttribute) {
                Matrix33<Double> m = ((M33dAttribute)a).getValue();
                out.print(toString(m));
            }
            else if (a instanceof M44fAttribute) {
                Matrix44<Float> m = ((M44fAttribute)a).getValue();
                out.print(toString(m));
            }
            else if (a instanceof M44dAttribute) {
                Matrix44<Double> m = ((M44dAttribute)a).getValue();
                out.print(toString(m));
            }
            else if (a instanceof PreviewImageAttribute) {
                PreviewImage p = ((PreviewImageAttribute)a).getValue();
                out.printf(": %d by %d pixels", p.getWidth(), p.getHeight());
            }
            else if (a instanceof StringAttribute) {
                String s = ((StringAttribute)a).getValue();
                out.printf(": \"%s\"", s);
            }
            else if (a instanceof StringVectorAttribute) {
                List<String> lst = ((StringVectorAttribute)a).getValue();
                out.print(':');
                for (String s : lst) {
                    out.printf("\n    \"%s\"", s);
                }
            }
            else if (a instanceof RationalAttribute) {
                Rational r = ((RationalAttribute)a).getValue();
                out.printf(": %s (%g)", r.toString(), r.doubleValue());
            }
            else if (a instanceof TileDescriptionAttribute) {
                TileDescription t = ((TileDescriptionAttribute)a).getValue();
                out.printf(":%n    %s", toString(t.mode));
                out.printf("%n    tile size %d by %d pixels", t.xSize, t.ySize);
                if (!t.mode.equals(TileDescription.LevelMode.ONE_LEVEL)) {
                    out.printf("%n    level sizes rounded %s",
                            toString(t.roundingMode));
                }
            }
            else if (a instanceof TimeCodeAttribute) {
                TimeCode t = ((TimeCodeAttribute)a).getValue();
                out.printf(":%n%s", toString(t));
            }
            else if (a instanceof TypedAttribute) {
                out.printf(": %s", ((TypedAttribute<?>)a).getValue());
            }
            else if (a instanceof OpaqueAttribute) {
                out.printf(": <opaque> [%d]",
                        ((OpaqueAttribute)a).getSize());
            }
            
            out.println();
        }
        out.println();
    }
    
    public static void main(String[] args) throws IOException {
        if (args.length < 1) {
            System.err.println("Error: missing arguments.");
            System.err.printf("Usage: java %s imagefile [imagefile ...]%n",
                    EXRHeaderApp.class.getName());
            System.exit(1);
        }
        try {
            for (String imagefile : args) {
                File file = new File(imagefile);
                printInfo(System.out, file);
            }
        }
        catch (Throwable t) {
            System.err.println(t.toString());
            t.printStackTrace(System.err);
            System.exit(1);
        }
    }
}

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

package edu.cornell.graphics.exr;

import edu.cornell.graphics.exr.attributes.Attribute;
import edu.cornell.graphics.exr.attributes.ChromaticitiesAttribute;
import edu.cornell.graphics.exr.attributes.EnvMapAttribute;
import edu.cornell.graphics.exr.attributes.FloatAttribute;
import edu.cornell.graphics.exr.attributes.KeyCodeAttribute;
import edu.cornell.graphics.exr.attributes.M44fAttribute;
import edu.cornell.graphics.exr.attributes.RationalAttribute;
import edu.cornell.graphics.exr.attributes.StringAttribute;
import edu.cornell.graphics.exr.attributes.StringVectorAttribute;
import edu.cornell.graphics.exr.attributes.TimeCodeAttribute;
import edu.cornell.graphics.exr.attributes.V2fAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Matrix44;
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import java.util.List;

/**
 * Optional Standard Attributes - these attributes are "optional"
 * because not every image file header has them, but they define a
 * "standard" way to represent commonly used data in the file header.
 * 
 * <p>The current OpenEXR standard attributes are:
 * <ul>
 * <li><b>chromaticities:</b> for RGB images, specifies the CIE (x,y)
 * chromaticities of the primaries and the white point.</li>
 * 
 * <li><b>whiteLuminance:</b> for RGB images, defines the luminance, in Nits
 * (candelas per square meter) of the RGB value {@code (1.0, 1.0, 1.0)}.
 * If the {@code chromaticities} and the {@code whiteLuminance} of an RGB image
 * are known, then it is possible to convert the image's pixels from RGB
 * to CIE XYZ tristimulus values.</li>
 * 
 * <li><b>adoptedNeutral:</b> specifies the CIE (x,y) coordinates that should
 * be considered neutral during color rendering. Pixels in the image
 * file whose (x,y) coordinates match the {@code adoptedNeutral} value should
 * be mapped to neutral values on the display.</li>
 * 
 * <li><b>renderingTransform, lookModTransform:</b> specify the names of the
 * CTL functions that implement the intended color rendering and look
 * modification transforms for this image.</li>
 * 
 * <li><b>xDensity:</b> horizontal output density, in pixels per inch.
 * The image's vertical output density is {@code xDensity * pixelAspectRatio}.
 * </li>
 * <li><b>owner:</b> name of the owner of the image.</li>
 * 
 * <li><b>comments:</b> additional image information in human-readable
 * form, for example a verbal description of the image.</li>
 * 
 * <li><b>capData:</b> the date when the image was created or captured,
 * in local time, and formatted as
 * <blockquote><pre>YYYY:MM:DD hh:mm:ss</pre></blockquote>
 * where {@code YYYY} is the year (4 digits, e.g. 2003), {@code MM} is the month
 * (2 digits, 01, 02, ... 12), {@code DD} is the day of the month (2 digits,
 * 01, 02, ... 31), {@code hh} is the hour (2 digits, 00, 01, ... 23) {@code mm}
 * is the minute, and {@code ss} is the second (2 digits, 00, 01, ... 59).</li>
 * 
 * <li><b>utcOffset:</b> offset of local time at {@code capDate} from
 * Universal Coordinated Time (UTC) in seconds:
 * <blockquote><pre>UTC = local time + utcOffset</pre></blockquote></li>
 * 
 * <li><b>longitude, latitude, altitude:</b> for image of real objects, the
 * location where the image was recorded. Longitude and latitude are
 * in degrees east of Greenwich and north of the equator. Altitude
 * is in meters above sea level. For example, Kathmandu, Nepal is
 * at longitude 85.317, latitude 27.717, altitude 1305.</li>
 * 
 * <li><b>focus:</b> the camera's focus distance, in meters.</li>
 * 
 * <li><b>expTime:</b> exposure time, in seconds.</li>
 * 
 * <li><b>aperture:</b> the camera's lens aperture, in f-stops (focal length
 * of the lens divided by the diameter of the iris opening.)</li>
 * 
 * <li><b>isoSpeed:</b> the ISO speed of the film or image sensor
 * that was used to record the image.</li>
 * 
 * <li><b>envmap:</b> if this attribute is present, the image represents
 * an environment map. The attribute's value defines how 3D
 * directions are mapped to 2D pixel locations. For details
 * see {@link EnvMap}.</li>
 * 
 * <li><b>keyCode:</b> for motion picture film frames. Identifies film
 * manufacturer, film type, film roll and frame position within
 * the roll.</li>
 * 
 * <li><b>timeCode:</b> time and control code.</li>
 * 
 * <li><b>wrapmodes:</b> determines how texture map images are extrapolated.
 * If an OpenEXR file is used as a texture map for 3D rendering,
 * texture coordinates (0.0, 0.0) and (1.0, 1.0) correspond to
 * the upper left and lower right corners of the data window.
 * If the image is mapped onto a surface with texture coordinates
 * outside the zero-to-one range, then the image must be extrapolated.
 * This attribute tells the renderer how to do this extrapolation.
 * The attribute contains either a pair of comma-separated keywords,
 * to specify separate extrapolation modes for the horizontal and
 * vertical directions; or a single keyword, to specify extrapolation
 * in both directions (e.g. "clamp,periodic" or "clamp".) Extra white
 * space surrounding the keywords is allowed, but should be ignored
 * by the renderer ("clamp, black " is equivalent to "clamp,black".)
 * The keywords listed below are predefined; some renderers may support
 * additional extrapolation modes:
 * <ul><li><tt>black</tt> - pixels outside the zero-to-one range are black.</li>
 *     <li><tt>clamp</tt> - texture coordinates less than 0.0 and greater
 *     than 1.0 are clamped to 0.0 and 1.0 respectively.</li>
 *     <li><tt>periodic</tt> - the texture image repeats periodically.</li>
 *      <li><tt>mirror</tt> - the texture image repeats periodically, but
 *      every other instance is mirrored.</li>
 * </ul></li>
 * 
 * <li><b>framesPerSecond:</b> defines the nominal playback frame rate for image
 * sequences, in frames per second. Every image in a sequence should
 * have a {@code framesPerSecond} attribute, and the attribute should be
 * the same for all images in the sequence. If an image sequence has
 * no {@code framesPerSecond} attribute, playback software should assume that
 * the frame rate for the sequence is 24 frames per second.
 * <p>In order to allow exact representation of NTSC frame and field rates,
 * {@code framesPerSecond} is stored as a rational number. A rational number is
 * a pair of integers, {@code n} and {@code d}, that represents the value
 * {@code n/d}.</p></li>
 * 
 * <li><b>multiView:</b> defines the view names for multi-view image files.
 * A multi-view image contains two or more views of the same scene,
 * as seen from different viewpoints, for example a left-eye and a
 * right-eye view for stereo displays. The {@code multiView} attribute
 * lists the names of the views in an image, and a naming convention
 * identifies the channels that belong to each view.</li>
 * 
 * <li><b>worldToCamera:</b> for images generated by 3D computer graphics
 * rendering, a matrix that transforms 3D points from the world to the camera
 * coordinate space of the renderer.
 * <p>The camera coordinate space is left-handed. Its origin indicates the 
 * location of the camera. The positive x and y axes correspond to the
 * "right" and "up" directions in the rendered image. The positive z axis
 * indicates the camera's viewing direction. (Objects in front of the camera
 * have positive z coordinates.)</p>
 * <p>Camera coordinate space in OpenEXR 
 * is the same as in Pixar's Renderman.</p></li>
 * 
 * <li><b>worldToNDC:</b> for images generated by 3D computer graphics 
 * rendering, a matrix that transforms 3D points from the world to the 
 * Normalized Device Coordinate (NDC) space of the renderer.
 * <p>NDC is a 2D coordinate space that corresponds to the image plane, with
 * positive x pointing to the right and positive y pointing down. The
 * coordinates (0, 0) and (1, 1) correspond to the upper left and lower right
 * corners of the OpenEXR display window.</p>
 * <p>To transform a 3D point in world space into a 2D point in NDC space,
 * multiply the 3D point by the {@code worldToNDC} matrix and discard the
 * z coordinate.</p>
 * <p>NDC space in OpenEXR is the same as in Pixar's Renderman.</p></li>
 * 
 * </ul>
 * </p>
 * 
 * @since 2.2
 */
public final class StandardAttributes {
    
    private StandardAttributes() {
        // empty
    }
    
    //=========================================================================
    // Chromaticities
    //=========================================================================
    
    /**
     * Add or update the {@code chromaticities} standard attribute
     * to the given header using a non-null {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code chromaticities} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addChromaticities(Header header, Chromaticities value) {
        header.insert("chromaticities", new ChromaticitiesAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code chromaticities} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code chromaticities} standard attribute
     */
    public static boolean hasChromaticities(Header header) {
        return header.findTypedAttribute("chromaticities",
                ChromaticitiesAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code chromaticities} standard attribute.
     * 
     *  <p>If the header does not have a {@code chromaticities} attribute,
     * that is if {@code header.hasChromaticities()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code chromaticities} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code chromaticities} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static ChromaticitiesAttribute
            getChromaticitiesAttribute(Header header) {
        return  (ChromaticitiesAttribute) header.getTypedAttribute(
                "chromaticities", ChromaticitiesAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code chromaticities} standard attribute.
     * 
     * <p>If the header does not have a {@code chromaticities} attribute,
     * that is if {@code header.hasChromaticities()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code chromaticities} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code chromaticities} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static Chromaticities getChromaticities(Header header) {
        return header.getTypedAttribute("chromaticities",
                ChromaticitiesAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // WhiteLuminance
    //=========================================================================
    
    /**
     * Add or update the {@code whiteLuminance} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code whiteLuminance} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addWhiteLuminance(Header header, float value) {
        header.insert("whiteLuminance", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code whiteLuminance} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code whiteLuminance} standard attribute
     */
    public static boolean hasWhiteLuminance(Header header) {
        return header.findTypedAttribute("whiteLuminance",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code whiteLuminance} standard attribute.
     * 
     *  <p>If the header does not have a {@code whiteLuminance} attribute,
     * that is if {@code header.hasWhiteLuminance()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code whiteLuminance} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code whiteLuminance} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getWhiteLuminanceAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "whiteLuminance", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code whiteLuminance} standard attribute.
     * 
     * <p>If the header does not have a {@code whiteLuminance} attribute,
     * that is if {@code header.hasWhiteLuminance()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code whiteLuminance} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code whiteLuminance} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getWhiteLuminance(Header header) {
        return header.getTypedAttribute("whiteLuminance",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // AdoptedNeutral
    //=========================================================================
    
    /**
     * Add or update the {@code adoptedNeutral} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code adoptedNeutral} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addAdoptedNeutral(Header header, Vector2<Float> value) {
        header.insert("adoptedNeutral", new V2fAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code adoptedNeutral} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code adoptedNeutral} standard attribute
     */
    public static boolean hasAdoptedNeutral(Header header) {
        return header.findTypedAttribute("adoptedNeutral",
                V2fAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code adoptedNeutral} standard attribute.
     * 
     *  <p>If the header does not have a {@code adoptedNeutral} attribute,
     * that is if {@code header.hasAdoptedNeutral()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code adoptedNeutral} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code adoptedNeutral} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static V2fAttribute
            getAdoptedNeutralAttribute(Header header) {
        return  (V2fAttribute) header.getTypedAttribute(
                "adoptedNeutral", V2fAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code adoptedNeutral} standard attribute.
     * 
     * <p>If the header does not have a {@code adoptedNeutral} attribute,
     * that is if {@code header.hasAdoptedNeutral()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code adoptedNeutral} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code adoptedNeutral} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static Vector2<Float> getAdoptedNeutral(Header header) {
        return header.getTypedAttribute("adoptedNeutral",
                V2fAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // RenderingTransform
    //=========================================================================
    
    /**
     * Add or update the {@code renderingTransform} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code renderingTransform} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addRenderingTransform(Header header, String value) {
        header.insert("renderingTransform", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code renderingTransform} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code renderingTransform} standard attribute
     */
    public static boolean hasRenderingTransform(Header header) {
        return header.findTypedAttribute("renderingTransform",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code renderingTransform} standard attribute.
     * 
     *  <p>If the header does not have a {@code renderingTransform} attribute,
     * that is if {@code header.hasRenderingTransform()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code renderingTransform} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code renderingTransform} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getRenderingTransformAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "renderingTransform", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code renderingTransform} standard attribute.
     * 
     * <p>If the header does not have a {@code renderingTransform} attribute,
     * that is if {@code header.hasRenderingTransform()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code renderingTransform} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code renderingTransform} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getRenderingTransform(Header header) {
        return header.getTypedAttribute("renderingTransform",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // LookModTransform
    //=========================================================================
    
    /**
     * Add or update the {@code lookModTransform} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code lookModTransform} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addLookModTransform(Header header, String value) {
        header.insert("lookModTransform", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code lookModTransform} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code lookModTransform} standard attribute
     */
    public static boolean hasLookModTransform(Header header) {
        return header.findTypedAttribute("lookModTransform",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code lookModTransform} standard attribute.
     * 
     *  <p>If the header does not have a {@code lookModTransform} attribute,
     * that is if {@code header.hasLookModTransform()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code lookModTransform} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code lookModTransform} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getLookModTransformAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "lookModTransform", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code lookModTransform} standard attribute.
     * 
     * <p>If the header does not have a {@code lookModTransform} attribute,
     * that is if {@code header.hasLookModTransform()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code lookModTransform} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code lookModTransform} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getLookModTransform(Header header) {
        return header.getTypedAttribute("lookModTransform",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // XDensity
    //=========================================================================
    
    /**
     * Add or update the {@code xDensity} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code xDensity} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addXDensity(Header header, float value) {
        header.insert("xDensity", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code xDensity} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code xDensity} standard attribute
     */
    public static boolean hasXDensity(Header header) {
        return header.findTypedAttribute("xDensity",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code xDensity} standard attribute.
     * 
     *  <p>If the header does not have a {@code xDensity} attribute,
     * that is if {@code header.hasXDensity()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code xDensity} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code xDensity} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getXDensityAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "xDensity", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code xDensity} standard attribute.
     * 
     * <p>If the header does not have a {@code xDensity} attribute,
     * that is if {@code header.hasXDensity()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code xDensity} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code xDensity} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getXDensity(Header header) {
        return header.getTypedAttribute("xDensity",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Owner
    //=========================================================================
    
    /**
     * Add or update the {@code owner} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code owner} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addOwner(Header header, String value) {
        header.insert("owner", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code owner} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code owner} standard attribute
     */
    public static boolean hasOwner(Header header) {
        return header.findTypedAttribute("owner",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code owner} standard attribute.
     * 
     *  <p>If the header does not have a {@code owner} attribute,
     * that is if {@code header.hasOwner()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code owner} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code owner} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getOwnerAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "owner", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code owner} standard attribute.
     * 
     * <p>If the header does not have a {@code owner} attribute,
     * that is if {@code header.hasOwner()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code owner} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code owner} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getOwner(Header header) {
        return header.getTypedAttribute("owner",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Comments
    //=========================================================================
    
    /**
     * Add or update the {@code comments} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code comments} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addComments(Header header, String value) {
        header.insert("comments", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code comments} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code comments} standard attribute
     */
    public static boolean hasComments(Header header) {
        return header.findTypedAttribute("comments",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code comments} standard attribute.
     * 
     *  <p>If the header does not have a {@code comments} attribute,
     * that is if {@code header.hasComments()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code comments} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code comments} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getCommentsAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "comments", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code comments} standard attribute.
     * 
     * <p>If the header does not have a {@code comments} attribute,
     * that is if {@code header.hasComments()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code comments} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code comments} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getComments(Header header) {
        return header.getTypedAttribute("comments",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // CapDate
    //=========================================================================
    
    /**
     * Add or update the {@code capDate} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code capDate} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addCapDate(Header header, String value) {
        header.insert("capDate", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code capDate} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code capDate} standard attribute
     */
    public static boolean hasCapDate(Header header) {
        return header.findTypedAttribute("capDate",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code capDate} standard attribute.
     * 
     *  <p>If the header does not have a {@code capDate} attribute,
     * that is if {@code header.hasCapDate()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code capDate} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code capDate} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getCapDateAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "capDate", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code capDate} standard attribute.
     * 
     * <p>If the header does not have a {@code capDate} attribute,
     * that is if {@code header.hasCapDate()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code capDate} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code capDate} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getCapDate(Header header) {
        return header.getTypedAttribute("capDate",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // UtcOffset
    //=========================================================================
    
    /**
     * Add or update the {@code utcOffset} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code utcOffset} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addUtcOffset(Header header, float value) {
        header.insert("utcOffset", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code utcOffset} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code utcOffset} standard attribute
     */
    public static boolean hasUtcOffset(Header header) {
        return header.findTypedAttribute("utcOffset",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code utcOffset} standard attribute.
     * 
     *  <p>If the header does not have a {@code utcOffset} attribute,
     * that is if {@code header.hasUtcOffset()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code utcOffset} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code utcOffset} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getUtcOffsetAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "utcOffset", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code utcOffset} standard attribute.
     * 
     * <p>If the header does not have a {@code utcOffset} attribute,
     * that is if {@code header.hasUtcOffset()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code utcOffset} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code utcOffset} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getUtcOffset(Header header) {
        return header.getTypedAttribute("utcOffset",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Longitude
    //=========================================================================
    
    /**
     * Add or update the {@code longitude} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code longitude} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addLongitude(Header header, float value) {
        header.insert("longitude", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code longitude} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code longitude} standard attribute
     */
    public static boolean hasLongitude(Header header) {
        return header.findTypedAttribute("longitude",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code longitude} standard attribute.
     * 
     *  <p>If the header does not have a {@code longitude} attribute,
     * that is if {@code header.hasLongitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code longitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code longitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getLongitudeAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "longitude", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code longitude} standard attribute.
     * 
     * <p>If the header does not have a {@code longitude} attribute,
     * that is if {@code header.hasLongitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code longitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code longitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getLongitude(Header header) {
        return header.getTypedAttribute("longitude",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Latitude
    //=========================================================================
    
    /**
     * Add or update the {@code latitude} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code latitude} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addLatitude(Header header, float value) {
        header.insert("latitude", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code latitude} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code latitude} standard attribute
     */
    public static boolean hasLatitude(Header header) {
        return header.findTypedAttribute("latitude",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code latitude} standard attribute.
     * 
     *  <p>If the header does not have a {@code latitude} attribute,
     * that is if {@code header.hasLatitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code latitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code latitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getLatitudeAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "latitude", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code latitude} standard attribute.
     * 
     * <p>If the header does not have a {@code latitude} attribute,
     * that is if {@code header.hasLatitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code latitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code latitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getLatitude(Header header) {
        return header.getTypedAttribute("latitude",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Altitude
    //=========================================================================
    
    /**
     * Add or update the {@code altitude} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code altitude} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addAltitude(Header header, float value) {
        header.insert("altitude", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code altitude} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code altitude} standard attribute
     */
    public static boolean hasAltitude(Header header) {
        return header.findTypedAttribute("altitude",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code altitude} standard attribute.
     * 
     *  <p>If the header does not have a {@code altitude} attribute,
     * that is if {@code header.hasAltitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code altitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code altitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getAltitudeAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "altitude", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code altitude} standard attribute.
     * 
     * <p>If the header does not have a {@code altitude} attribute,
     * that is if {@code header.hasAltitude()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code altitude} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code altitude} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getAltitude(Header header) {
        return header.getTypedAttribute("altitude",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Focus
    //=========================================================================
    
    /**
     * Add or update the {@code focus} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code focus} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addFocus(Header header, float value) {
        header.insert("focus", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code focus} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code focus} standard attribute
     */
    public static boolean hasFocus(Header header) {
        return header.findTypedAttribute("focus",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code focus} standard attribute.
     * 
     *  <p>If the header does not have a {@code focus} attribute,
     * that is if {@code header.hasFocus()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code focus} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code focus} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getFocusAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "focus", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code focus} standard attribute.
     * 
     * <p>If the header does not have a {@code focus} attribute,
     * that is if {@code header.hasFocus()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code focus} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code focus} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getFocus(Header header) {
        return header.getTypedAttribute("focus",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // ExpTime
    //=========================================================================
    
    /**
     * Add or update the {@code expTime} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code expTime} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addExpTime(Header header, float value) {
        header.insert("expTime", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code expTime} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code expTime} standard attribute
     */
    public static boolean hasExpTime(Header header) {
        return header.findTypedAttribute("expTime",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code expTime} standard attribute.
     * 
     *  <p>If the header does not have a {@code expTime} attribute,
     * that is if {@code header.hasExpTime()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code expTime} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code expTime} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getExpTimeAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "expTime", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code expTime} standard attribute.
     * 
     * <p>If the header does not have a {@code expTime} attribute,
     * that is if {@code header.hasExpTime()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code expTime} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code expTime} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getExpTime(Header header) {
        return header.getTypedAttribute("expTime",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Aperture
    //=========================================================================
    
    /**
     * Add or update the {@code aperture} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code aperture} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addAperture(Header header, float value) {
        header.insert("aperture", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code aperture} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code aperture} standard attribute
     */
    public static boolean hasAperture(Header header) {
        return header.findTypedAttribute("aperture",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code aperture} standard attribute.
     * 
     *  <p>If the header does not have a {@code aperture} attribute,
     * that is if {@code header.hasAperture()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code aperture} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code aperture} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getApertureAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "aperture", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code aperture} standard attribute.
     * 
     * <p>If the header does not have a {@code aperture} attribute,
     * that is if {@code header.hasAperture()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code aperture} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code aperture} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getAperture(Header header) {
        return header.getTypedAttribute("aperture",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // IsoSpeed
    //=========================================================================
    
    /**
     * Add or update the {@code isoSpeed} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code isoSpeed} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addIsoSpeed(Header header, float value) {
        header.insert("isoSpeed", new FloatAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code isoSpeed} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code isoSpeed} standard attribute
     */
    public static boolean hasIsoSpeed(Header header) {
        return header.findTypedAttribute("isoSpeed",
                FloatAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code isoSpeed} standard attribute.
     * 
     *  <p>If the header does not have a {@code isoSpeed} attribute,
     * that is if {@code header.hasIsoSpeed()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code isoSpeed} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code isoSpeed} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static FloatAttribute
            getIsoSpeedAttribute(Header header) {
        return  (FloatAttribute) header.getTypedAttribute(
                "isoSpeed", FloatAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code isoSpeed} standard attribute.
     * 
     * <p>If the header does not have a {@code isoSpeed} attribute,
     * that is if {@code header.hasIsoSpeed()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code isoSpeed} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code isoSpeed} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static float getIsoSpeed(Header header) {
        return header.getTypedAttribute("isoSpeed",
                FloatAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Envmap
    //=========================================================================
    
    /**
     * Add or update the {@code envmap} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code envmap} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addEnvMap(Header header, EnvMap value) {
        header.insert("envmap", new EnvMapAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code envmap} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code envmap} standard attribute
     */
    public static boolean hasEnvMap(Header header) {
        return header.findTypedAttribute("envmap",
                EnvMapAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code envmap} standard attribute.
     * 
     *  <p>If the header does not have a {@code envmap} attribute,
     * that is if {@code header.hasEnvMap()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code envmap} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code envmap} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static EnvMapAttribute
            getEnvMapAttribute(Header header) {
        return  (EnvMapAttribute) header.getTypedAttribute(
                "envmap", EnvMapAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code envmap} standard attribute.
     * 
     * <p>If the header does not have a {@code envmap} attribute,
     * that is if {@code header.hasEnvMap()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code envmap} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code envmap} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static EnvMap getEnvMap(Header header) {
        return header.getTypedAttribute("envmap",
                EnvMapAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // KeyCode
    //=========================================================================
    
    /**
     * Add or update the {@code keyCode} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code keyCode} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addKeyCode(Header header, KeyCode value) {
        header.insert("keyCode", new KeyCodeAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code keyCode} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code keyCode} standard attribute
     */
    public static boolean hasKeyCode(Header header) {
        return header.findTypedAttribute("keyCode",
                KeyCodeAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code keyCode} standard attribute.
     * 
     *  <p>If the header does not have a {@code keyCode} attribute,
     * that is if {@code header.hasKeyCode()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code keyCode} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code keyCode} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static KeyCodeAttribute
            getKeyCodeAttribute(Header header) {
        return  (KeyCodeAttribute) header.getTypedAttribute(
                "keyCode", KeyCodeAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code keyCode} standard attribute.
     * 
     * <p>If the header does not have a {@code keyCode} attribute,
     * that is if {@code header.hasKeyCode()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code keyCode} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code keyCode} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static KeyCode getKeyCode(Header header) {
        return header.getTypedAttribute("keyCode",
                KeyCodeAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // TimeCode
    //=========================================================================
    
    /**
     * Add or update the {@code timeCode} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code timeCode} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addTimeCode(Header header, TimeCode value) {
        header.insert("timeCode", new TimeCodeAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code timeCode} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code timeCode} standard attribute
     */
    public static boolean hasTimeCode(Header header) {
        return header.findTypedAttribute("timeCode",
                TimeCodeAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code timeCode} standard attribute.
     * 
     *  <p>If the header does not have a {@code timeCode} attribute,
     * that is if {@code header.hasTimeCode()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code timeCode} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code timeCode} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static TimeCodeAttribute
            getTimeCodeAttribute(Header header) {
        return  (TimeCodeAttribute) header.getTypedAttribute(
                "timeCode", TimeCodeAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code timeCode} standard attribute.
     * 
     * <p>If the header does not have a {@code timeCode} attribute,
     * that is if {@code header.hasTimeCode()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code timeCode} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code timeCode} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static TimeCode getTimeCode(Header header) {
        return header.getTypedAttribute("timeCode",
                TimeCodeAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // Wrapmodes
    //=========================================================================
    
    /**
     * Add or update the {@code wrapmodes} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code wrapmodes} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addWrapmodes(Header header, String value) {
        header.insert("wrapmodes", new StringAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code wrapmodes} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code wrapmodes} standard attribute
     */
    public static boolean hasWrapmodes(Header header) {
        return header.findTypedAttribute("wrapmodes",
                StringAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code wrapmodes} standard attribute.
     * 
     *  <p>If the header does not have a {@code wrapmodes} attribute,
     * that is if {@code header.hasWrapmodes()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code wrapmodes} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code wrapmodes} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringAttribute
            getWrapmodesAttribute(Header header) {
        return  (StringAttribute) header.getTypedAttribute(
                "wrapmodes", StringAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code wrapmodes} standard attribute.
     * 
     * <p>If the header does not have a {@code wrapmodes} attribute,
     * that is if {@code header.hasWrapmodes()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code wrapmodes} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code wrapmodes} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static String getWrapmodes(Header header) {
        return header.getTypedAttribute("wrapmodes",
                StringAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // FramesPerSecond
    //=========================================================================
    
    /**
     * Add or update the {@code framesPerSecond} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code framesPerSecond} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addFramesPerSecond(Header header, Rational value) {
        header.insert("framesPerSecond", new RationalAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code framesPerSecond} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code framesPerSecond} standard attribute
     */
    public static boolean hasFramesPerSecond(Header header) {
        return header.findTypedAttribute("framesPerSecond",
                RationalAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code framesPerSecond} standard attribute.
     * 
     *  <p>If the header does not have a {@code framesPerSecond} attribute,
     * that is if {@code header.hasFramesPerSecond()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code framesPerSecond} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code framesPerSecond} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static RationalAttribute
            getFramesPerSecondAttribute(Header header) {
        return  (RationalAttribute) header.getTypedAttribute(
                "framesPerSecond", RationalAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code framesPerSecond} standard attribute.
     * 
     * <p>If the header does not have a {@code framesPerSecond} attribute,
     * that is if {@code header.hasFramesPerSecond()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code framesPerSecond} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code framesPerSecond} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static Rational getFramesPerSecond(Header header) {
        return header.getTypedAttribute("framesPerSecond",
                RationalAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // MultiView
    //=========================================================================
    
    /**
     * Add or update the {@code multiView} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code multiView} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addMultiView(Header header, List<String> value) {
        header.insert("multiView", new StringVectorAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code multiView} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code multiView} standard attribute
     */
    public static boolean hasMultiView(Header header) {
        return header.findTypedAttribute("multiView",
                StringVectorAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code multiView} standard attribute.
     * 
     *  <p>If the header does not have a {@code multiView} attribute,
     * that is if {@code header.hasMultiView()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code multiView} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code multiView} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static StringVectorAttribute
            getMultiViewAttribute(Header header) {
        return  (StringVectorAttribute) header.getTypedAttribute(
                "multiView", StringVectorAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code multiView} standard attribute.
     * 
     * <p>If the header does not have a {@code multiView} attribute,
     * that is if {@code header.hasMultiView()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code multiView} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code multiView} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static List<String> getMultiView(Header header) {
        return header.getTypedAttribute("multiView",
                StringVectorAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // WorldToCamera
    //=========================================================================
    
    /**
     * Add or update the {@code worldToCamera} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code worldToCamera} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addWorldToCamera(Header header, Matrix44<Float> value) {
        header.insert("worldToCamera", new M44fAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code worldToCamera} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code worldToCamera} standard attribute
     */
    public static boolean hasWorldToCamera(Header header) {
        return header.findTypedAttribute("worldToCamera",
                M44fAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code worldToCamera} standard attribute.
     * 
     *  <p>If the header does not have a {@code worldToCamera} attribute,
     * that is if {@code header.hasWorldToCamera()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code worldToCamera} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code worldToCamera} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static M44fAttribute
            getWorldToCameraAttribute(Header header) {
        return  (M44fAttribute) header.getTypedAttribute(
                "worldToCamera", M44fAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code worldToCamera} standard attribute.
     * 
     * <p>If the header does not have a {@code worldToCamera} attribute,
     * that is if {@code header.hasWorldToCamera()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code worldToCamera} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code worldToCamera} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static Matrix44<Float> getWorldToCamera(Header header) {
        return header.getTypedAttribute("worldToCamera",
                M44fAttribute.class).getValue();
    }
    
    
    //=========================================================================
    // WorldToNDC
    //=========================================================================
    
    /**
     * Add or update the {@code worldToNDC} standard attribute
     * to the given header using {@code value}.
     * 
     * @param header a non-null {@code header}
     * @param value the new value of the {@code worldToNDC} attribute
     * @see Header#insert(String, Attribute) 
     */
    public static void addWorldToNDC(Header header, Matrix44<Float> value) {
        header.insert("worldToNDC", new M44fAttribute(value));
    }
    
    /**
     * Returns {@code true} if {@code header} contains the
     * {@code worldToNDC} standard attribute.
     * 
     * @param header a non-null {@code Header}
     * @return {@code true} if {@code header} contains the
     *         {@code worldToNDC} standard attribute
     */
    public static boolean hasWorldToNDC(Header header) {
        return header.findTypedAttribute("worldToNDC",
                M44fAttribute.class) != null;
    }
    
    /**
     * Returns a reference to an existing
     * {@code worldToNDC} standard attribute.
     * 
     *  <p>If the header does not have a {@code worldToNDC} attribute,
     * that is if {@code header.hasWorldToNDC()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to an existing
     *         {@code worldToNDC} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code worldToNDC} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static M44fAttribute
            getWorldToNDCAttribute(Header header) {
        return  (M44fAttribute) header.getTypedAttribute(
                "worldToNDC", M44fAttribute.class);
    }
    
    /**
     * Returns a reference to the current value of an existing
     * {@code worldToNDC} standard attribute.
     * 
     * <p>If the header does not have a {@code worldToNDC} attribute,
     * that is if {@code header.hasWorldToNDC()} is {@code false},
     * the method throws {@code IllegalArgumentException}.</p>
     * 
     * @param header a non-null {@code Header}
     * @return a reference to the current value of the
     *         {@code worldToNDC} standard attribute
     * @throws IllegalArgumentException if {@code header} does not have a
     *         {@code worldToNDC} standard attribute
     * @see Header#getTypedAttribute(String, Class) 
     */
    public static Matrix44<Float> getWorldToNDC(Header header) {
        return header.getTypedAttribute("worldToNDC",
                M44fAttribute.class).getValue();
    }
    
}

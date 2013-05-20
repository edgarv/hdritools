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
import edu.cornell.graphics.exr.attributes.AttributeFactory;
import edu.cornell.graphics.exr.attributes.Box2iAttribute;
import edu.cornell.graphics.exr.attributes.ChannelListAttribute;
import edu.cornell.graphics.exr.attributes.CompressionAttribute;
import edu.cornell.graphics.exr.attributes.FloatAttribute;
import edu.cornell.graphics.exr.attributes.LineOrderAttribute;
import edu.cornell.graphics.exr.attributes.OpaqueAttribute;
import edu.cornell.graphics.exr.attributes.TileDescriptionAttribute;
import edu.cornell.graphics.exr.attributes.TypedAttribute;
import edu.cornell.graphics.exr.attributes.V2fAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import edu.cornell.graphics.exr.io.XdrInput;
import java.io.IOException;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Set;
import java.util.TreeMap;

/**
 * Abstraction of the header describing an OpenEXR file.
 * 
 * <p>The header contains the set of attributes and channels; it is used for
 * writing new files and contains all the information from existing ones.
 * An instance of {@code Header} provides iterator access to the named
 * attributes. While all aspects of a file are specified as attributes, this
 * class provides accessors and setters for the attributes which have to be
 * present in all OpenEXR files.</p>
 * 
 * <p>This class is modeled after {@code ImfHeader} in the origial C++ OpenEXR
 * library.</p>
 */
public final class Header implements Iterable<Entry<String, Attribute>> {
    
    private final static HashSet<String> predefinedAttributes;
    
    private final TreeMap<String, Attribute> map = new TreeMap<>();
    
    private static final AttributeFactory factory =
            AttributeFactory.newDefaultFactory();
    
    static {
        predefinedAttributes = new HashSet<>(8);
        predefinedAttributes.add("displayWindow");
        predefinedAttributes.add("dataWindow");
        predefinedAttributes.add("pixelAspectRatio");
        predefinedAttributes.add("screenWindowCenter");
        predefinedAttributes.add("screenWindowWidth");
        predefinedAttributes.add("lineOrder");
        predefinedAttributes.add("compression");
        predefinedAttributes.add("channels");
    }

    /**
     * Returns a read-only iterator over the existing attributes.
     * 
     * <p>The elements are ordered by attribute name. The returned entries
     * represent snapshots of the attributes at the time they were produced.
     * 
     * @return Returns an iterator over the existing attributes.
     */
    @Override
    public Iterator<Entry<String, Attribute>> iterator() {
        return Collections.unmodifiableSet(map.entrySet()).iterator();
    }
    
    /**
     * Returns a read-only {@link Set} view of the attribute names contained
     * in this header. The set's iterator returns the attribute names in
     * ascending order.
     * 
     * @return a read-only {@code Set} view of the attribute names contained
     *         in this header.
     */
    public Set<String> attributeNameSet() {
        return Collections.unmodifiableSet(map.keySet());
    }
    
    /**
     * Default constructor. Creates a file with width and height 64, with the
     * attributes set as in {@link #Header(int, int) }.
     */
    public Header() {
        this(64, 64);
    }
    
    /**
     * Creates a header where the display window and the data window are both
     * set to <tt>[0,0] x [width-1, height-1]</tt>, with an empty channel list.
     * 
     * <p>The other predefined attributes are initialized as follows:
     * <table>
     *   <tr><th>Attribute</th><th>Value</th></tr>
     *   <tr><td><tt>pixelAspectRatio</tt></td><td>1.0</td>
     *   <tr><td><tt>screenWindowCenter</tt></td><td>(0.0, 0.0)</td>
     *   <tr><td><tt>screenWindowWidth</tt></td><td>1.0</td>
     *   <tr><td><tt>lineOrder</tt></td><td>{@link LineOrder#INCREASING_Y}</td>
     *   <tr><td><tt>compression</tt></td><td>{@link Compression#ZIP}</td>
     * </table>
     * 
     * @param width positive width of the file.
     * @param height positive height of the file.
     * @throws IllegalArgumentException if either parameter is less than 1.
     */
    public Header(int width, int height) throws IllegalArgumentException {
        if (width < 1) {
            throw new IllegalArgumentException("Illegal width: "  + width);
        } else if (height < 1) {
            throw new IllegalArgumentException("Illegal height: " + height);
        }
        final int bW = width-1, bH = height-1;
        insert("displayWindow", new Box2iAttribute(new Box2<>(0, 0, bW, bH)));
        insert("dataWindow",    new Box2iAttribute(new Box2<>(0, 0, bW, bH)));
        insert("pixelAspectRatio",   new FloatAttribute(1.0f));
        insert("screenWindowCenter", new V2fAttribute(new Vector2<>(0.f, 0.f)));
        insert("screenWindowWidth",  new FloatAttribute(1.0f));
        insert("lineOrder",   new LineOrderAttribute(LineOrder.INCREASING_Y));
        insert("compression", new CompressionAttribute(Compression.ZIP));
        insert("channels", new ChannelListAttribute(new ChannelList()));
    }
    
    /**
     * Copy constructor. Creates a deep copy of the other header attributes.
     * 
     * @param other the header to duplicate.
     * @throws IllegalArgumentException if {@code other} is {@code null}.
     */
    public Header(Header other) throws IllegalArgumentException {
        if (other == null) {
            throw new IllegalArgumentException("null header");
        }
        for (Entry<String, Attribute> attr : other) {
            assert attr.getKey() != null && !attr.getKey().isEmpty();
            assert attr.getValue() != null;
            insert(attr.getKey(), attr.getValue().clone());
        }
    }
    
    /**
     * <p>Add an attribute to the header. If no attribute with name {@code n}
     * exists, a new attribute with name {@code n} and the same type as
     * {@code attr}, is added, and the value of {@code attr} is copied into
     * the new attribute.</p>
     * 
     * <p>If an attribute with name {@code n} exists, and its type is the same
     * as {@code attr}, the value of {@code attr} is copied into this attribute.
     * </p>
     * <p>If an attribute with name {@code n} exists, and its type is different
     * from {@code attr} an {@code EXRTypeException} is thrown.
     *
     * @param n the non-empty name of the attribute.
     * @param attr the non-null attribute to add.
     * @throws IllegalArgumentException if {@code n} or {@code attr} are empty
     *         or {@code null}.
     * @throws EXRTypeException if an attribute with name {@code n} exists, and
     *         its type is different from {@code attr}.
     */
    public void insert(String n, Attribute attr) throws
            IllegalArgumentException, EXRTypeException {
        if (n == null || n.isEmpty()) {
            throw new IllegalArgumentException("Image attribute name cannot be "
                    + " an empty string.");
        } else if (attr == null) {
            throw new IllegalArgumentException("The attribute cannot be null");
        } else if (attr instanceof TypedAttribute && 
                ((TypedAttribute<?>)attr).getValue() == null) {
            throw new IllegalArgumentException("The typed attribute value " +
                    "cannot be null");
        }
        
        Attribute oldValue = map.get(n);
        if (oldValue != null && !oldValue.typeName().equals(attr.typeName())) {
            throw new EXRTypeException(String.format("Cannot assign a value of "
                    + "type \"%s\" to image attribute \"%s\" of type \"%s\".",
                    attr.typeName(), n, oldValue.typeName()));
        }
        Attribute myAttr = attr.clone();
        map.put(n, myAttr);
    }
    
    /**
     * If an attribute with the given name exists, then it is removed from the
     * map of present attributes. Otherwise this function is a "no-op".
     * 
     * @param name the name of the attribute to remove.
     * @throws IllegalArgumentException if {@code name} is either {@code null}
     *         or empty, or it corresponds to a predefined attribute.
     */
    public void erase(String name) throws IllegalArgumentException {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("invalid attribute name");
        } else if (predefinedAttributes.contains(name)) {
            throw new IllegalArgumentException("cannot erase predefined " +
                    "attribute: " + name);
        }
        map.remove(name);
    }
    
    /**
     * Get the parameter class at runtime through reflection. See:
     * http://blog.xebia.com/2009/02/07/acessing-generic-types-at-runtime-in-java/
     */
    private static Class<?> getParamClass(Class<? extends TypedAttribute<?>> c){
        Class<?> clazz = c;
        while (clazz.getSuperclass() != null &&
              !clazz.getSuperclass().equals(TypedAttribute.class)) {
            clazz = clazz.getSuperclass();
        }
        if (clazz.getSuperclass() == null) {
            throw new IllegalStateException("Not instance of TypedAttribute");
        }
        ParameterizedType type=(ParameterizedType) clazz.getGenericSuperclass();
        final Type argType = type.getActualTypeArguments()[0];
        final Class<?> argClass;
        if (argType instanceof Class) {
            argClass = (Class<?>) argType;
        } else if (argType instanceof ParameterizedType) {
            argClass = (Class<?>) ((ParameterizedType) argType).getRawType();
        } else {
            throw new IllegalStateException("Invalid type: " + argType);
        }
        return argClass;
    }
    
    /**
     * Returns a reference to the typed attribute with name {@code n} and value
     * type {@code T}. If no attribute with name {@code n} exists, an
     * {@code IllegalArgumentException} is thrown. If an attribute with name
     * {@code n} exists, but its value type is not {@code T}, an
     * {@code EXRTypeException} is thrown.
     * 
     * @param name non-empty name of the desired attribute.
     * @param cls the class of the value in the attribute.
     * @return the typed attribute with name {@code n} and value type {@code T}.
     * @throws EXRTypeException if an attribute with name {@code n} exists, but
     *         its value type is not {@code T}.
     * @throws IllegalArgumentException if no attribute with name
     *         {@code n} exists.
     */
    public <T> TypedAttribute<T> getTypedAttribute(String name,
            Class<? extends TypedAttribute<T>> cls) throws
            IllegalArgumentException, EXRTypeException {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Image attribute name cannot be "
                    + " an empty string.");
        }
        Attribute attr = map.get(name);
        if (attr == null) {
            throw new IllegalArgumentException("Attribute not found: " + name);
        }
        if (!(attr instanceof TypedAttribute)) {
            throw new EXRTypeException("Not a typed attribute");
        }
        
        TypedAttribute<?> typedAttr = (TypedAttribute<?>) attr;
        Object value = typedAttr.getValue();
        if (value == null) {
            throw new IllegalStateException("null attribute value!");
        }
        
        final Class<?> argClass = getParamClass(cls);
        if (argClass.isInstance(value)) {
            return (TypedAttribute<T>) typedAttr;
        } else {
            throw new EXRTypeException("The attribute does not match the " +
                    "requested type: expected: " + argClass.getCanonicalName() +
                    ", actual: " + value.getClass().getCanonicalName());
        }
    }
    
    /**
     * Returns a reference to the typed attribute with name {@code n} and value
     * type {@code T}, or {@code null} if no attribute with name {@code n}
     * <tt>and</tt> type {@code T} exists.
     * 
     * @param name non-empty name of the desired attribute.
     * @param cls the class of the value in the attribute.
     * @return the typed attribute with name {@code n} and value type {@code T}
     *         or {@code null}.
     */
    public <T> TypedAttribute<T> findTypedAttribute(String name,
            Class<? extends TypedAttribute<T>> cls) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Image attribute name cannot be "
                    + " an empty string.");
        }
        Attribute attr = map.get(name);
        if (attr == null || !(attr instanceof TypedAttribute)) {
            return null;
        }
        
        TypedAttribute<?> typedAttr = (TypedAttribute<?>) attr;
        Object value = typedAttr.getValue();
        if (value == null) {
            throw new IllegalStateException("null attribute value!");
        }
        
        final Class<?> aClass = getParamClass(cls);
        return (TypedAttribute<T>)(aClass.isInstance(value) ? typedAttr : null);
    }

    //--------------------------------
    // Access to predefined attributes
    //--------------------------------
    
    /**
     * Returns a reference to the value of the <tt>displayWindow</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>displayWindow</tt> attribute
     */
    public Box2<Integer> getDisplayWindow() {
        return getTypedAttribute("displayWindow",
                Box2iAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>dataWindow</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>dataWindow</tt> attribute
     */
    public Box2<Integer> getDataWindow() {
        return getTypedAttribute("dataWindow",
                Box2iAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>pixelAspectRatio</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>pixelAspectRatio</tt>
     * attribute
     */
    public Float getPixelAspectRatio() {
        return getTypedAttribute("pixelAspectRatio",
                FloatAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>screenWindowCenter</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>screenWindowCenter</tt>
     * attribute
     */
    public Vector2<Float> getScreenWindowCenter() {
        return getTypedAttribute("screenWindowCenter",
                V2fAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>screenWindowWidth</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>screenWindowWidth</tt> 
     * attribute
     */
    public Float getScreenWindowWidth() {
        return getTypedAttribute("screenWindowWidth",
                FloatAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>channels</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>channels</tt> attribute
     */
    public ChannelList getChannels() {
        return getTypedAttribute("channels",
                ChannelListAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>lineOrder</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>lineOrder</tt> attribute
     */
    public LineOrder getLineOrder() {
        return getTypedAttribute("lineOrder",
                LineOrderAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>compression</tt>
     * predefined attribute.
     * 
     * @return a reference to the value of the <tt>compression</tt> attribute
     */
    public Compression getCompression() {
        return getTypedAttribute("compression",
                CompressionAttribute.class).getValue();
    }
    
    /**
     * Returns a reference to the value of the <tt>tiles</tt> attribute
     * attribute if and only if {@link #hasTileDescription() } 
     * returns {@code true}.
     * 
     * <p>The "tiles" attribute must be present in any tiled image file.
     * When present, it describes various properties of the tiles that make up
     * the file. If the "tiles" attribute is not present this method
     * throws an exception.</p>
     * 
     * @return a reference to the value of the <tt>tiles</tt> attribute
     */
    public TileDescription getTileDescription() {
        return getTypedAttribute("tiles",
                TileDescriptionAttribute.class).getValue();
    }
    
    /**
     * Returns whether the header contains a {@link TileDescriptionAttribute}
     * whose name is "tiles".
     * 
     * <p>The "tiles" attribute must be present in any tiled image file.
     * When present, it describes various properties of the tiles that make up
     * the file. The implementation simply returns
     * {@code findTypedAttribute("tiles",TileDescriptionAttribute.class)!=null}.
     * </p>
     * 
     * @return {@code true} if the header contains a
     * {@link TileDescriptionAttribute} whose name is "tiles", {@code false}
     * otherwise.
     */
    public boolean hasTileDescription() {
        return findTypedAttribute("tiles",TileDescriptionAttribute.class)!=null;
    }
    
    /**
     * Examines the header, and throws an {@code IllegalArgumentException} if
     * the header is non-null and it finds something wrong (e.g. empty display
     * window, negative pixel aspect ratio, unknown compression scheme, etc.)
     *
     * @param h the header to verify
     * @param isTiled whether the header is supposed to have tiles or not
     * @throws IllegalArgumentException if there is something wrong
     *         with the file
     */
    public static void sanityCheck(Header h, boolean isTiled) throws
            IllegalArgumentException {
        if (h == null) {
            return;
        }
        //
        // The display window and the data window must each
        // contain at least one pixel. In addition, the
        // coordinates of the window corners must be small
        // enough to keep expressions like max-min+1 or
        // max+min from overflowing.
        //
        
        Box2<Integer> displayWindow = h.getDisplayWindow();
        if (displayWindow.xMin.compareTo(displayWindow.xMax) > 0 ||
            displayWindow.yMin.compareTo(displayWindow.yMax) > 0 ||
            displayWindow.xMin.compareTo(-Integer.MAX_VALUE / 2) <= 0 ||
            displayWindow.yMin.compareTo(-Integer.MAX_VALUE / 2) <= 0 ||
            displayWindow.xMax.compareTo( Integer.MAX_VALUE / 2) <= 0 ||
            displayWindow.yMax.compareTo( Integer.MAX_VALUE / 2) <= 0)
        {
            throw new IllegalArgumentException("Invalid display window "
                    + "in image header.");            
        }
        
        Box2<Integer> dataWindow = h.getDataWindow();
        if (dataWindow.xMin.compareTo(dataWindow.xMax) > 0 ||
            dataWindow.yMin.compareTo(dataWindow.yMax) > 0 ||
            dataWindow.xMin.compareTo(-Integer.MAX_VALUE / 2) <= 0 ||
            dataWindow.yMin.compareTo(-Integer.MAX_VALUE / 2) <= 0 ||
            dataWindow.xMax.compareTo( Integer.MAX_VALUE / 2) <= 0 ||
            dataWindow.yMax.compareTo( Integer.MAX_VALUE / 2) <= 0)
        {
            throw new IllegalArgumentException("Invalid data window "
                    + "in image header.");            
        }
        
        //
        // The pixel aspect ratio must be greater than 0.
        // In applications, numbers like the the display or
        // data window dimensions are likely to be multiplied
        // or divided by the pixel aspect ratio; to avoid
        // arithmetic exceptions, we limit the pixel aspect
        // ratio to a range that is smaller than theoretically
        // possible (real aspect ratios are likely to be close
        // to 1.0 anyway).
        //
        
        float pixelAspectRatio = h.getPixelAspectRatio();
        
        final float MIN_PIXEL_ASPECT_RATIO = 1e-6f;
        final float MAX_PIXEL_ASPECT_RATIO = 1e+6f;

        if (pixelAspectRatio < MIN_PIXEL_ASPECT_RATIO ||
            pixelAspectRatio > MAX_PIXEL_ASPECT_RATIO) {
            throw new IllegalArgumentException("Invalid pixel aspect ratio "
                    + "in image header.");
        }
        
        //
        // The screen window width must not be less than 0.
        // The size of the screen window can vary over a wide
        // range (fish-eye lens to astronomical telescope),
        // so we can't limit the screen window width to a
        // small range.
        //
        
        float screenWindowWidth = h.getScreenWindowWidth();
        if (screenWindowWidth < 0.0f) {
            throw new IllegalArgumentException("Invalid screen window width "
                    + "in image header.");
        }
        
        //
        // If the file is tiled, verify that the tile description has resonable
        // values and check to see if the lineOrder is one of the predefined 3.
        // If the file is not tiled, then the lineOrder can only be INCREASING_Y
        // or DECREASING_Y.
        //
        
        LineOrder lineOrder = h.getLineOrder();
        if (isTiled) {
            if (!h.hasTileDescription()) {
                throw new IllegalArgumentException("Tiled image has no tile "
                        + "description attribute.");
            }
            TileDescription tileDesc = h.getTileDescription();
            if (tileDesc.xSize <= 0 || tileDesc.ySize <= 0) {
                throw new IllegalArgumentException("Invalid tile size in "
                        + "image header.");
            }
            if (tileDesc.mode == null) {
                throw new IllegalArgumentException("Invalid level mode in "
                        + "image header.");
            }
            if (tileDesc.roundingMode == null) {
                throw new IllegalArgumentException("Invalid rounding mode in "
                        + "image header.");
            }
            if (lineOrder == null) {
                throw new IllegalArgumentException("Invalid line order in "
                        + "image header.");
            }
        }
        else {
            if (lineOrder == null || 
                (!lineOrder.equals(LineOrder.INCREASING_Y) &&
                 !lineOrder.equals(LineOrder.INCREASING_Y))) {
                throw new IllegalArgumentException("Invalid line order in "
                        + "image header.");
            }
        }
        
        if (h.getCompression() == null) {
            throw new IllegalArgumentException("Invalid compression in "
                    + "image header.");
        }
       
        //
        // Check the channel list:
        //
        // If the file is tiled then for each channel, the type must be one of
        // the predefined values, and the x and y sampling must both be 1.
        //
        // If the file is not tiled then for each channel, the type must be one
        // of the predefined values, the x and y coordinates of the data
        // window's upper left corner must be divisible by the x and y
        // subsampling factors, and the width and height of the data window must
        // be divisible by the x and y subsampling factors.
        //
        
        ChannelList channels = h.getChannels();
        if (isTiled) {
            for (ChannelList.ChannelListElement elem : channels) {
                final String name = elem.getName();
                final Channel c   = elem.getChannel();
                if (c.type == null) {
                    throw new IllegalArgumentException("Pixel type of \"" +
                            name + "\" image channel is invalid.");
                }
                if (c.xSampling != 1) {
                    throw new IllegalArgumentException("The x subsampling "
                            + "factor for the \"" + name + "\" channel "
                            + "is not 1.");
                }
                if (c.ySampling != 1) {
                    throw new IllegalArgumentException("The y subsampling "
                            + "factor for the \"" + name + "\" channel "
                            + "is not 1.");
                }
            }
        }
        else {
            for (ChannelList.ChannelListElement elem : channels) {
                final String name = elem.getName();
                final Channel c   = elem.getChannel();
                if (c.type == null) {
                    throw new IllegalArgumentException("Pixel type of \"" +
                            name + "\" image channel is invalid.");
                }
                if (c.xSampling < 1) {
                    throw new IllegalArgumentException("The x subsampling "
                            + "factor for the \"" + name + "\" channel "
                            + "is invalid.");
                }
                if (c.ySampling < 1) {
                    throw new IllegalArgumentException("The y subsampling "
                            + "factor for the \"" + name + "\" channel "
                            + "is invalid.");
                }
                if (dataWindow.xMin.intValue() % c.xSampling != 0) {
                    throw new IllegalArgumentException("The minimum x "
                            + "coordinate of the image's data window is not a "
                            + "multiple of the x subsampling factor of the \"" 
                            + name + "\" channel.");
                }
                if (dataWindow.yMin.intValue() % c.ySampling != 0) {
                    throw new IllegalArgumentException("The minimum y "
                            + "coordinate of the image's data window is not a "
                            + "multiple of the y subsampling factor of the \"" 
                            + name + "\" channel.");
                }
                
                int width = dataWindow.xMax.intValue() -
                            dataWindow.xMin.intValue() + 1;
                if (width % c.xSampling != 0) {
                    throw new IllegalArgumentException("The number of pixels "
                            + "per row in the image's data window is not a "
                            + "multiple of the x subsampling factor of the \"" 
                            + name + "\" channel.");
                }
                
                int height = dataWindow.yMax.intValue() -
                             dataWindow.yMin.intValue() + 1;
                if (height % c.ySampling != 0) {
                    throw new IllegalArgumentException("The number of pixels "
                            + "per column in the image's data window is not a "
                            + "multiple of the y subsampling factor of the \"" 
                            + name + "\" channel.");
                }
            }
        }
    }

    
    
    public void readFrom(XdrInput input, int version) throws 
            EXRIOException, IOException {
        
        final int maxNameLength = EXRVersion.getMaxNameLength(version);
        
        // Read all attributes
        for(;;) {
            // Read the name of the attribute.
            // A zero-length attribute name indicates the end of the header.
            String name = input.readNullTerminatedUTF8(maxNameLength);
            if (name.isEmpty()) {
                break;
            }

            // Read the attribute type and the size of the attribute value.
            String type = input.readNullTerminatedUTF8(maxNameLength);
            int size    = input.readInt();
            
            Attribute currentAttr = map.get(name);
            if (currentAttr != null) {
                // The attribute already exists (for example,
                // because it is a predefined attribute).
                // Read the attribute's new value from the file.
                if (!type.equals(currentAttr.typeName())) {
                    throw new EXRIOException("Unexpected type for "
                            + "image attribute \"" + name + "\".");
                }
                currentAttr.readValueFrom(input, size, version);
            }
            else {
                // The new attribute does not exist yet.
                // If the attribute type is of a known type,
                // read the attribute value. If the attribute
                // is of an unknown type, read its value and
                // store it as an OpaqueAttribute.
                Attribute attr;
                if (factory.isKnownType(type)) {
                    attr = factory.newAttribute(type);
                } else {
                    attr = new OpaqueAttribute(type);
                }
                attr.readValueFrom(input, size, version);
                insert(name, attr);
            }
        }
    }

}

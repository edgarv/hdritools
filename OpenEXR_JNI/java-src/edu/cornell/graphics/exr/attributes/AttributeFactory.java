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

import edu.cornell.graphics.exr.attributes.Attribute.AttributeCreator;
import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * Instance in charge of creating new attributes after the corresponding
 * creators have been specified. The methods of this class are thread safe,
 * however it does not enforce that each creator is reentrant as well
 * (they should be).
 */
public class AttributeFactory {
    
    private final HashMap<String, Attribute.AttributeCreator> typeMap =
            new HashMap<String, Attribute.AttributeCreator>();
    
    private final ReentrantReadWriteLock rwl = new ReentrantReadWriteLock();
    
    /**
     * Creates a new attribute for which {@link Attribute#typeName()} matches
     * the {@code typeName} argument, using one of the registered instances of
     * {@link AttributeFactory}. If the type name is not know it throws
     * {@code IllegalArgumentException}.
     * 
     * @param typeName the name representing the type of the desired attribute.
     * @return a non-null instance of the given type of attribute.
     * @throws IllegalArgumentException if the type is not known.
     * @see #registerAttributeType(java.lang.String, edu.cornell.graphics.exr.attributes.Attribute.AttributeCreator) 
     */
    public Attribute newAttribute(String typeName)
            throws IllegalArgumentException {
        rwl.readLock().lock();
        try {
            AttributeCreator creator = typeMap.get(typeName);
            if (creator != null) {
                return creator.newInstance();
            } else {
                throw new IllegalArgumentException("There is no creator " +
                        "registered for type " + typeName);
            }
        }
        finally {
            rwl.readLock().unlock();
        }
    }
    
    /**
     * Return <tt>true</tt> if this factory has a creator register to handle
     * the {@code typeName} attribute type, so that calling
     * {@link #newAttribute(java.lang.String) } with the same {@code typeName}
     * should succeed.
     * 
     * @param typeName the type name whose presence in the factory is tested.
     * @return <tt>true</tt> if this factory has a creator registered for this
     *         attribute type.
     */
    public boolean isKnownType(String typeName) {
        if (typeName == null || typeName.isEmpty()) {
            throw new IllegalArgumentException("Invalid type name: "+typeName);
        }
        rwl.readLock().lock();
        try {
            return typeMap.containsKey(typeName);
        } finally {
            rwl.readLock().unlock();
        }
    }
    
    /**
     * Register a new attribute creator which will be used when
     * {@link #newAttribute(java.lang.String) } is called with the type name
     * {@code typeName}. If there is already a handler registered for the
     * attribute type it throws {@code IllegalArgumentException}.
     * 
     * @param typeName the type name associated with the creator instance.
     * @param creator the instance which will create attributes of the
     *        specified type.
     * @throws IllegalArgumentException if there is already a handler for the
     *        specified attribute type.
     */
    public void registerAttributeType(String typeName,
            Attribute.AttributeCreator creator) throws IllegalArgumentException{
        if (typeName == null || typeName.isEmpty()) {
            throw new IllegalArgumentException("Invalid type name: "+typeName);
        }
        if (creator == null) {
            throw new IllegalArgumentException("Null factory");
        }
        rwl.writeLock().lock();
        try {
            if (!typeMap.containsKey(typeName)) {
                typeMap.put(typeName, creator);
            } else {
                throw new IllegalArgumentException("There is already a handler "
                        + "for the attribute type " + typeName);
            }
        }
        finally {
            rwl.writeLock().unlock();
        }
    }
    
    /**
     * Creates a new instance of an anonymous creator which creates a new
     * instance using the default constructor through reflection.
     * 
     * @param cls the class of an {@code Attribute} which provides a public
     *        default constructor.
     * @return a creator which invokes the default constructor of the Attribute.
     */
    public static Attribute.AttributeCreator newDefaultCreator(
            final Class<? extends Attribute> cls) {
        try {
            return new AttributeCreator() {
                final Constructor<? extends Attribute> c = cls.getConstructor();
                
                @Override
                public Attribute newInstance() {
                    try {
                        Attribute attr = c.newInstance();
                        return attr;
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                }
            };
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
    
    // Helper method to create the default attribute factory.
    private void register(Class<? extends Attribute> cls) {
        AttributeCreator creator = newDefaultCreator(cls);
        Attribute tmp = creator.newInstance();
        String typeName = tmp.typeName();
        registerAttributeType(typeName, creator);
    }
    
    /**
     * Creates a new instance of an {@code AttributeFactory} which handles
     * the default known attribute types for OpenEXR 1.7.1 image files. The
     * returned factory instance may be further modified by the clients.
     * 
     * @return a new instance of {@code AttributeFactory} which handles the
     *         default OpenEXR 1.7.1 attribute types.
     */
    public static AttributeFactory newDefaultFactory() {
        AttributeFactory f = new AttributeFactory();
        
        f.register(Box2fAttribute.class);
        f.register(Box2iAttribute.class);
        f.register(ChannelListAttribute.class);
        f.register(ChromaticitiesAttribute.class);
        f.register(CompressionAttribute.class);
        f.register(DoubleAttribute.class);
        f.register(EnvMapAttribute.class);
        f.register(FloatAttribute.class);
        f.register(IntegerAttribute.class);
        f.register(KeyCodeAttribute.class);
        f.register(LineOrderAttribute.class);
        f.register(M33dAttribute.class);
        f.register(M33fAttribute.class);
        f.register(M44dAttribute.class);
        f.register(M44fAttribute.class);
        f.register(PreviewImageAttribute.class);
        f.register(RationalAttribute.class);
        f.register(StringAttribute.class);
        f.register(StringVectorAttribute.class);
        f.register(TileDescriptionAttribute.class);
        f.register(TimeCodeAttribute.class);
        f.register(V2dAttribute.class);
        f.register(V2fAttribute.class);
        f.register(V2iAttribute.class);
        f.register(V3dAttribute.class);
        f.register(V3fAttribute.class);
        f.register(V3iAttribute.class);
        
        return f;
    }
}

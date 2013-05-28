/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.cornell.graphics.exr;

import edu.cornell.graphics.exr.attributes.Attribute;
import edu.cornell.graphics.exr.attributes.ChannelListAttribute;
import edu.cornell.graphics.exr.attributes.CompressionAttribute;
import edu.cornell.graphics.exr.attributes.M33fAttribute;
import edu.cornell.graphics.exr.attributes.TypedAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.ilmbaseto.Matrix33;
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import edu.cornell.graphics.exr.io.EXRFileInputStream;
import edu.cornell.graphics.exr.io.InputFileInfo;
import edu.cornell.graphics.exr.io.XdrInput;
import java.io.IOException;
import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Tests reading the header from a known file
 */
public class HeaderBasicTest {
    
    private final static String RES_FILENMAME =
            "edu/cornell/graphics/exr/resources/test-piz-rgb.exr";
    
    private static Header instance;
    
    public HeaderBasicTest() {
        // Nothing to do
    }
    
    @BeforeClass
    public static void setUpClass() throws IOException, URISyntaxException {
        // As a basic test, they all use the same instance
        java.net.URL url = ClassLoader.getSystemResource(RES_FILENMAME);
        if (url == null) {
            fail("Could not open source resource: " + RES_FILENMAME);
        }
        final Path path = Paths.get(url.toURI());
        try (EXRFileInputStream is = new EXRFileInputStream(path)) {
            XdrInput input = new XdrInput(is);
            InputFileInfo info = new InputFileInfo(input);
            instance = info.getHeader();
        }
    }
    
    @Before
    public void setUp() {
        assertNotNull(instance);
    }

    /**
     * Test of iterator method, of class Header.
     */
    @Test
    public void testIterator() {
        System.out.println("iterator");
        Iterator result = instance.iterator();
        assertNotNull(result);
        
        // Reference attribute names
        HashSet<String> expected = new HashSet<>(8);
        expected.add("channels");
        expected.add("compression");
        expected.add("dataWindow");
        expected.add("displayWindow");
        expected.add("lineOrder");
        expected.add("pixelAspectRatio");
        expected.add("screenWindowCenter");
        expected.add("screenWindowWidth");
        
        HashSet<String> actual = new HashSet<>(8);
        for (Map.Entry<String, Attribute> e : instance) {
            final String name    = e.getKey();
            final Attribute attr = e.getValue();
            assertFalse(name.isEmpty());
            assertNotNull(attr);
            assertFalse(attr.typeName().isEmpty());
            System.out.printf("%32s : %s%n", name, attr);
            assertTrue(actual.add(name));
        }
        
        assertEquals(expected.size(), actual.size());
        assertFalse(expected.retainAll(actual));
        assertFalse(actual.retainAll(expected));
    }
    
    private void testInsertBadType(Header header, String name, Attribute attr) {
        try {
            header.insert(name, attr);
            fail("insert had to fail");
        } catch(RuntimeException ex) {
            if (!(ex instanceof EXRTypeException)) {
                fail("Unexpected exception: " + ex.getMessage());
                throw ex;
            }
        }
    }
    
    private void testInsertBadArg(Header header, String name, Attribute attr) {
        try {
            header.insert(name, attr);
            fail("insert had to fail");
        } catch(RuntimeException ex) {
            if (!(ex instanceof IllegalArgumentException)) {
                fail("Unexpected exception: " + ex.getMessage());
                throw ex;
            }
        }
    }

    /**
     * Test of insert method, of class Header.
     */
    @Test
    public void testInsert() {
        System.out.println("insert");

        // Header with default attributes
        Header header = new Header();
        M33fAttribute m33Attrib = new M33fAttribute();
        
        // Name nor attribute may be null
        testInsertBadArg(header, null, null);
        testInsertBadArg(header, "", null);
        testInsertBadArg(header, "", m33Attrib);
        testInsertBadArg(header, "matrix", null);
        testInsertBadArg(header, "matrix", m33Attrib);
        
        m33Attrib.setValue(new Matrix33<>(1.0f));
        
        // Cannot insert an attribute with an exising name and different type
        testInsertBadType(header, "displayWindow",      m33Attrib);
        testInsertBadType(header, "dataWindow",         m33Attrib);
        testInsertBadType(header, "pixelAspectRatio",   m33Attrib);
        testInsertBadType(header, "screenWindowCenter", m33Attrib);
        testInsertBadType(header, "screenWindowWidth",  m33Attrib);
        testInsertBadType(header, "lineOrder",          m33Attrib);
        testInsertBadType(header, "compression",        m33Attrib);
        testInsertBadType(header, "channels",           m33Attrib);
        
        // Fresh, non null attribute has to succeed
        assertNull(header.findTypedAttribute("matrix", M33fAttribute.class));
        header.insert("matrix", m33Attrib);
        assertNotNull(header.findTypedAttribute("matrix", M33fAttribute.class));
        
        Matrix33<Float> m1 = m33Attrib.getValue();
        Matrix33<Float> m2 = new Matrix33<>(2.0f);

        // The attribute is fully copied
        assertEquals(m33Attrib, header.getTypedAttribute("matrix",
                M33fAttribute.class));
        assertNotSame(m33Attrib, header.getTypedAttribute("matrix",
                M33fAttribute.class));
        assertNotSame(m1, header.getTypedAttribute("matrix",
                M33fAttribute.class).getValue());
        assertEquals(m1, header.getTypedAttribute("matrix",
                M33fAttribute.class).getValue());
        
        m33Attrib.setValue(m2);
        assertEquals(m1, header.getTypedAttribute("matrix",
                M33fAttribute.class).getValue());
        
        // The value is fully copied
        m1.m00 *= -2.0f;
        assertFalse(m1.equals(header.getTypedAttribute("matrix",
                M33fAttribute.class).getValue()));
        header.getTypedAttribute("matrix", M33fAttribute.class).getValue()
                .m00 *= -2.0f;
        assertEquals(m1, header.getTypedAttribute("matrix",
                M33fAttribute.class).getValue());
    }

    /**
     * Test of getTypedAttribute method, of class Header.
     */
    @Test(expected=IllegalArgumentException.class)
    public void testGetTypedAttribute1() {
        System.out.println("getTypedAttribute1");
        instance.getTypedAttribute("channelsX", ChannelListAttribute.class);
    }
    
    @Test(expected=EXRTypeException.class)
    public void testGetTypedAttribute2() {
        System.out.println("getTypedAttribute2");
        instance.getTypedAttribute("channels", CompressionAttribute.class);
    }
    
    public void testGetTypedAttribute3() {
        System.out.println("getTypedAttribute3");
        TypedAttribute<Compression> result = instance.getTypedAttribute(
                "compression", CompressionAttribute.class);
        assertNotNull(result);
        assertEquals(Compression.PIZ, result.getValue());
    }

    /**
     * Test of findTypedAttribute method, of class Header.
     */
    @Test
    public void testFindTypedAttribute() {
        System.out.println("findTypedAttribute");
        TypedAttribute<?> result;
        result = instance.findTypedAttribute("channelsX",
                ChannelListAttribute.class);
        assertNull(result);
        result = instance.findTypedAttribute("channels",
                CompressionAttribute.class);
        assertNull(result);
        result = instance.findTypedAttribute("compression",
                CompressionAttribute.class);
        assertNotNull(result);
        assertEquals(Compression.PIZ, result.getValue());
    }

    /**
     * Test of getDisplayWindow method, of class Header.
     */
    @Test
    public void testGetDisplayWindow() {
        System.out.println("getDisplayWindow");
        Box2<Integer> expResult = new Box2<>(0, 0, 1023, 31);
        Box2<Integer> result = instance.getDisplayWindow();
        assertEquals(expResult, result);
    }

    /**
     * Test of getDataWindow method, of class Header.
     */
    @Test
    public void testGetDataWindow() {
        System.out.println("getDataWindow");
        Box2<Integer> expResult = new Box2<>(0, 0, 1023, 31);
        Box2<Integer> result = instance.getDataWindow();
        assertEquals(expResult, result);
    }

    /**
     * Test of getPixelAspectRatio method, of class Header.
     */
    @Test
    public void testGetPixelAspectRatio() {
        System.out.println("getPixelAspectRatio");
        Float expResult = 1.0f;
        Float result = instance.getPixelAspectRatio();
        assertEquals(expResult, result);
    }

    /**
     * Test of getScreenWindowCenter method, of class Header.
     */
    @Test
    public void testGetScreenWindowCenter() {
        System.out.println("getScreenWindowCenter");
        Vector2<Float> expResult = new Vector2<>(0.0f, 0.0f);
        Vector2<Float> result = instance.getScreenWindowCenter();
        assertEquals(expResult, result);
    }

    /**
     * Test of getScreenWindowWidth method, of class Header.
     */
    @Test
    public void testGetScreenWindowWidth() {
        System.out.println("getScreenWindowWidth");
        Float expResult = 1.0f;
        Float result = instance.getScreenWindowWidth();
        assertEquals(expResult, result);
    }

    /**
     * Test of getChannels method, of class Header.
     */
    @Test
    public void testGetChannels() {
        System.out.println("getChannels");
        final ChannelList chlst = instance.getChannels();
        assertNotNull(chlst);
        assertFalse(chlst.isEmpty());
        assertEquals(3, chlst.size());
        assertTrue(chlst.containsChannel("R"));
        assertTrue(chlst.containsChannel("G"));
        assertTrue(chlst.containsChannel("B"));
        assertFalse(chlst.containsChannel("A"));
        assertFalse(chlst.containsChannel("Y"));
        assertFalse(chlst.containsChannel("Z"));
        
        // The channels have to be iterable in ascending order
        String previous = "";
        for (ChannelList.ChannelListElement elem : chlst) {
            final String name = elem.getName();
            assertFalse(name.isEmpty());
            assertTrue(previous.compareTo(name) < 0);
            previous = name;
            
            // In the test file all channels have the same specs
            final Channel channel = elem.getChannel();
            assertNotNull(channel);
            assertEquals(PixelType.HALF, channel.type);
            assertEquals(1, channel.xSampling);
            assertEquals(1, channel.ySampling);
            assertFalse(channel.pLinear);
        }
        
        HashSet<String> names = new HashSet<>(chlst.nameSet());
        assertTrue(names.remove("R"));
        assertTrue(names.remove("G"));
        assertTrue(names.remove("B"));
        assertTrue(names.isEmpty());
    }

    /**
     * Test of getLineOrder method, of class Header.
     */
    @Test
    public void testGetLineOrder() {
        System.out.println("getLineOrder");
        LineOrder expResult = LineOrder.INCREASING_Y;
        LineOrder result = instance.getLineOrder();
        assertEquals(expResult, result);
    }

    /**
     * Test of getCompression method, of class Header.
     */
    @Test
    public void testGetCompression() {
        System.out.println("getCompression");
        Compression expResult = Compression.PIZ;
        Compression result = instance.getCompression();
        assertEquals(expResult, result);
    }

    /**
     * Test of getTileDescription method, of class Header.
     */
    @Test(expected=IllegalArgumentException.class)
    public void testGetTileDescription() {
        System.out.println("getTileDescription");
        TileDescription result = instance.getTileDescription();
        assertNull(result);
    }

    /**
     * Test of hasTileDescription method, of class Header.
     */
    @Test
    public void testHasTileDescription() {
        System.out.println("hasTileDescription");
        assertFalse(instance.hasTileDescription());
    }

    /**
     * Test of sanityCheck method, of class Header.
     */
    @Test(expected=IllegalArgumentException.class)
    public void testSanityCheck1() {
        System.out.println("sanityCheck");
        Header.sanityCheck(instance, true);
    }

}

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package edu.cornell.graphics.exr;

import edu.cornell.graphics.exr.attributes.Attribute;
import edu.cornell.graphics.exr.attributes.ChannelListAttribute;
import edu.cornell.graphics.exr.attributes.CompressionAttribute;
import edu.cornell.graphics.exr.attributes.TypedAttribute;
import edu.cornell.graphics.exr.ilmbaseto.Box2;
import edu.cornell.graphics.exr.ilmbaseto.Vector2;
import edu.cornell.graphics.exr.io.EXRBufferedDataInput;
import edu.cornell.graphics.exr.io.InputFileInfo;
import java.io.IOException;
import java.io.InputStream;
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
    public static void setUpClass() throws IOException {
        // As a basic test, they all use the same instance
        InputStream is = ClassLoader.getSystemResourceAsStream(RES_FILENMAME);
        if (is == null) {
            fail("Could not open source resource: " + RES_FILENMAME);
        }
        EXRBufferedDataInput input = new EXRBufferedDataInput(is);
        InputFileInfo info = new InputFileInfo(input);
        instance = info.getHeader();
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
        HashSet<String> expected = new HashSet<String>(8);
        expected.add("channels");
        expected.add("compression");
        expected.add("dataWindow");
        expected.add("displayWindow");
        expected.add("lineOrder");
        expected.add("pixelAspectRatio");
        expected.add("screenWindowCenter");
        expected.add("screenWindowWidth");
        
        HashSet<String> actual = new HashSet<String>(8);
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

    /**
     * Test of insert method, of class Header.
     */
    @Test
    public void testInsert() {
        System.out.println("insert");
        // TODO review the generated test code and remove the default call to fail.
        fail("The test case is a prototype.");
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
        Box2<Integer> expResult = new Box2<Integer>(0, 0, 1023, 31);
        Box2<Integer> result = instance.getDisplayWindow();
        assertEquals(expResult, result);
    }

    /**
     * Test of getDataWindow method, of class Header.
     */
    @Test
    public void testGetDataWindow() {
        System.out.println("getDataWindow");
        Box2<Integer> expResult = new Box2<Integer>(0, 0, 1023, 31);
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
        Vector2<Float> expResult = new Vector2<Float>(0.0f, 0.0f);
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
            assertEquals(Channel.PixelType.HALF, channel.type);
            assertEquals(1, channel.xSampling);
            assertEquals(1, channel.ySampling);
            assertFalse(channel.pLinear);
        }
        
        HashSet<String> names = new HashSet<String>(chlst.nameSet());
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

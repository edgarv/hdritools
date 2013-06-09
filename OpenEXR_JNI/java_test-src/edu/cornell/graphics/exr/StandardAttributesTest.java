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

package edu.cornell.graphics.exr;

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
import edu.cornell.graphics.exr.io.ByteBufferInputStream;
import edu.cornell.graphics.exr.io.EXRByteArrayOutputStream;
import edu.cornell.graphics.exr.io.XdrInput;
import edu.cornell.graphics.exr.io.XdrOutput;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import static org.junit.Assert.*;
import org.junit.Before;
import org.junit.Test;

public class StandardAttributesTest {
    
    private Header header;
    
    /** Initialize each test with a fresh header */
    @Before
    public void setUp() {
        header = new Header(640, 480);
    }
    
    private void checkSerialization() {
        EXRByteArrayOutputStream outStream = new EXRByteArrayOutputStream();
        try {
            final int hash = header.hashCode();
            XdrOutput out = new XdrOutput(outStream);
            header.writeTo(out);
            assertEquals(hash, header.hashCode());
            
            final byte[] buffer = outStream.array();
            ByteBufferInputStream inputStream = new ByteBufferInputStream(
                    ByteBuffer.wrap(buffer, 0, (int) outStream.position()));
            XdrInput in = new XdrInput(inputStream);
            Header h = new Header();
            assertFalse(header.equals(h));
            final int version = header.version();
            h.readFrom(in, version);
            assertEquals(header, h);
            assertEquals(hash, h.hashCode());
        } catch (Exception ex) {
            throw new RuntimeException(ex.getMessage(), ex);
        }
    }
    
    
    //=========================================================================
    // Chromaticities
    //=========================================================================

    @Test
    public void testChromaticities() {
        System.out.println("chromaticities");
        assertFalse(StandardAttributes.hasChromaticities(header));
        
        final Chromaticities value = new Chromaticities();
        value.redX   = 0.6400f;
        value.redY   = 0.3300f;
        value.greenX = 0.3000f;
        value.greenY = 0.6000f;
        value.blueX  = 0.1500f;
        value.blueY  = 0.0600f;
        value.whiteX = 0.3127f;
        value.whiteY = 0.3290f;
        
        StandardAttributes.addChromaticities(header, value);
        assertTrue(StandardAttributes.hasChromaticities(header));
        ChromaticitiesAttribute attr =
                StandardAttributes.getChromaticitiesAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        Chromaticities hValue = StandardAttributes.getChromaticities(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.redX *= 2.0f;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testChromaticitiesBadAttr() {
        ChromaticitiesAttribute attr =
                StandardAttributes.getChromaticitiesAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testChromaticitiesBadValue() {
        Chromaticities value = StandardAttributes.getChromaticities(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // WhiteLuminance
    //=========================================================================
    
    @Test
    public void testWhiteLuminance() {
        System.out.println("whiteLuminance");
        assertFalse(StandardAttributes.hasWhiteLuminance(header));
        
        final float value = 1.875f;
        StandardAttributes.addWhiteLuminance(header, value);
        assertTrue(StandardAttributes.hasWhiteLuminance(header));
        FloatAttribute attr =
                StandardAttributes.getWhiteLuminanceAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getWhiteLuminance(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWhiteLuminanceBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getWhiteLuminanceAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWhiteLuminanceBadValue() {
        float value = StandardAttributes.getWhiteLuminance(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // AdoptedNeutral
    //=========================================================================
    
    @Test
    public void testAdoptedNeutral() {
        System.out.println("adoptedNeutral");
        assertFalse(StandardAttributes.hasAdoptedNeutral(header));
        
        final Vector2<Float> value = new Vector2<>();
        value.x = 0.7612327648117558f;
        value.y = 0.31377950940949295f;
        
        StandardAttributes.addAdoptedNeutral(header, value);
        assertTrue(StandardAttributes.hasAdoptedNeutral(header));
        V2fAttribute attr =
                StandardAttributes.getAdoptedNeutralAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        Vector2<Float> hValue = StandardAttributes.getAdoptedNeutral(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.x *= 2.0f;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testAdoptedNeutralBadAttr() {
        V2fAttribute attr =
                StandardAttributes.getAdoptedNeutralAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testAdoptedNeutralBadValue() {
        Vector2<Float> value = StandardAttributes.getAdoptedNeutral(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // RenderingTransform
    //=========================================================================
    
    @Test
    public void testRenderingTransform() {
        System.out.println("renderingTransform");
        assertFalse(StandardAttributes.hasRenderingTransform(header));
        
        final String value = "asdfasfda";
        
        StandardAttributes.addRenderingTransform(header, value);
        assertTrue(StandardAttributes.hasRenderingTransform(header));
        StringAttribute attr =
                StandardAttributes.getRenderingTransformAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getRenderingTransform(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testRenderingTransformBadAttr() {
        StringAttribute attr =
                StandardAttributes.getRenderingTransformAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testRenderingTransformBadValue() {
        String value = StandardAttributes.getRenderingTransform(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // LookModTransform
    //=========================================================================
    
    @Test
    public void testLookModTransform() {
        System.out.println("lookModTransform");
        assertFalse(StandardAttributes.hasLookModTransform(header));
        
        final String value = "532a861f-9fc3-4674-b15e-e91ee3198369";
        
        StandardAttributes.addLookModTransform(header, value);
        assertTrue(StandardAttributes.hasLookModTransform(header));
        StringAttribute attr =
                StandardAttributes.getLookModTransformAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getLookModTransform(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLookModTransformBadAttr() {
        StringAttribute attr =
                StandardAttributes.getLookModTransformAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLookModTransformBadValue() {
        String value = StandardAttributes.getLookModTransform(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // XDensity
    //=========================================================================
    
    @Test
    public void testXDensity() {
        System.out.println("xDensity");
        assertFalse(StandardAttributes.hasXDensity(header));
        
        final float value = 41.860561559132904f;
        StandardAttributes.addXDensity(header, value);
        assertTrue(StandardAttributes.hasXDensity(header));
        FloatAttribute attr =
                StandardAttributes.getXDensityAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getXDensity(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testXDensityBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getXDensityAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testXDensityBadValue() {
        float value = StandardAttributes.getXDensity(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Owner
    //=========================================================================
    
    @Test
    public void testOwner() {
        System.out.println("owner");
        assertFalse(StandardAttributes.hasOwner(header));
        
        final String value = "de6e1961-9e3d-4a9c-8edd-d9482157dda4";
        
        StandardAttributes.addOwner(header, value);
        assertTrue(StandardAttributes.hasOwner(header));
        StringAttribute attr =
                StandardAttributes.getOwnerAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getOwner(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testOwnerBadAttr() {
        StringAttribute attr =
                StandardAttributes.getOwnerAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testOwnerBadValue() {
        String value = StandardAttributes.getOwner(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Comments
    //=========================================================================
   
    @Test
    public void testComments() {
        System.out.println("comments");
        assertFalse(StandardAttributes.hasComments(header));
        
        final String value = "46894b3a-3db2-4d09-8d4f-18d3fbeb4c6e" + "  ipsum"
                + "\n" + "b1aca3f0-e9ce-4324-9b48-f9dd346e1e35";
        
        StandardAttributes.addComments(header, value);
        assertTrue(StandardAttributes.hasComments(header));
        StringAttribute attr =
                StandardAttributes.getCommentsAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getComments(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testCommentsBadAttr() {
        StringAttribute attr =
                StandardAttributes.getCommentsAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testCommentsBadValue() {
        String value = StandardAttributes.getComments(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // CapDate
    //=========================================================================
   
    @Test
    public void testCapDate() {
        System.out.println("capDate");
        assertFalse(StandardAttributes.hasCapDate(header));
        
        final String value = "2013:06:09 09:43:13";
        
        StandardAttributes.addCapDate(header, value);
        assertTrue(StandardAttributes.hasCapDate(header));
        StringAttribute attr =
                StandardAttributes.getCapDateAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getCapDate(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testCapDateBadAttr() {
        StringAttribute attr =
                StandardAttributes.getCapDateAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testCapDateBadValue() {
        String value = StandardAttributes.getCapDate(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // UtcOffset
    //=========================================================================
   
    @Test
    public void testUtcOffset() {
        System.out.println("utcOffset");
        assertFalse(StandardAttributes.hasUtcOffset(header));
        
        final float value = 14400.0f;
        StandardAttributes.addUtcOffset(header, value);
        assertTrue(StandardAttributes.hasUtcOffset(header));
        FloatAttribute attr =
                StandardAttributes.getUtcOffsetAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getUtcOffset(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testUtcOffsetBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getUtcOffsetAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testUtcOffsetBadValue() {
        float value = StandardAttributes.getUtcOffset(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Longitude
    //=========================================================================
   
    @Test
    public void testLongitude() {
        System.out.println("longitude");
        assertFalse(StandardAttributes.hasLongitude(header));
        
        final float value = -48.36394786297572f;
        StandardAttributes.addLongitude(header, value);
        assertTrue(StandardAttributes.hasLongitude(header));
        FloatAttribute attr =
                StandardAttributes.getLongitudeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getLongitude(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLongitudeBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getLongitudeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLongitudeBadValue() {
        float value = StandardAttributes.getLongitude(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Latitude
    //=========================================================================
   
    @Test
    public void testLatitude() {
        System.out.println("latitude");
        assertFalse(StandardAttributes.hasLatitude(header));
        
        final float value = 43.03534769457764f;
        StandardAttributes.addLatitude(header, value);
        assertTrue(StandardAttributes.hasLatitude(header));
        FloatAttribute attr =
                StandardAttributes.getLatitudeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getLatitude(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLatitudeBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getLatitudeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testLatitudeBadValue() {
        float value = StandardAttributes.getLatitude(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Altitude
    //=========================================================================
   
    @Test
    public void testAltitude() {
        System.out.println("altitude");
        assertFalse(StandardAttributes.hasAltitude(header));
        
        final float value = -26.03580046795487f;
        StandardAttributes.addAltitude(header, value);
        assertTrue(StandardAttributes.hasAltitude(header));
        FloatAttribute attr =
                StandardAttributes.getAltitudeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getAltitude(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testAltitudeBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getAltitudeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testAltitudeBadValue() {
        float value = StandardAttributes.getAltitude(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Focus
    //=========================================================================
   
    @Test
    public void testFocus() {
        System.out.println("focus");
        assertFalse(StandardAttributes.hasFocus(header));
        
        final float value = 47.68518829275175f;
        StandardAttributes.addFocus(header, value);
        assertTrue(StandardAttributes.hasFocus(header));
        FloatAttribute attr =
                StandardAttributes.getFocusAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getFocus(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testFocusBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getFocusAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testFocusBadValue() {
        float value = StandardAttributes.getFocus(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // ExpTime
    //=========================================================================
   
    @Test
    public void testExpTime() {
        System.out.println("expTime");
        assertFalse(StandardAttributes.hasExpTime(header));
        
        final float value = 24.27055674192256f;
        StandardAttributes.addExpTime(header, value);
        assertTrue(StandardAttributes.hasExpTime(header));
        FloatAttribute attr =
                StandardAttributes.getExpTimeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getExpTime(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testExpTimeBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getExpTimeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testExpTimeBadValue() {
        float value = StandardAttributes.getExpTime(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Aperture
    //=========================================================================
   
    @Test
    public void testAperture() {
        System.out.println("aperture");
        assertFalse(StandardAttributes.hasAperture(header));
        
        final float value = 22.737508778532856f;
        StandardAttributes.addAperture(header, value);
        assertTrue(StandardAttributes.hasAperture(header));
        FloatAttribute attr =
                StandardAttributes.getApertureAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getAperture(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testApertureBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getApertureAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testApertureBadValue() {
        float value = StandardAttributes.getAperture(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // IsoSpeed
    //=========================================================================
   
    @Test
    public void testIsoSpeed() {
        System.out.println("isoSpeed");
        assertFalse(StandardAttributes.hasIsoSpeed(header));
        
        final float value = 17.499677145024393f;
        StandardAttributes.addIsoSpeed(header, value);
        assertTrue(StandardAttributes.hasIsoSpeed(header));
        FloatAttribute attr =
                StandardAttributes.getIsoSpeedAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue(), 0.0);
        
        final Float hValue = StandardAttributes.getIsoSpeed(header);
        assertNotNull(hValue);
        assertEquals(value, hValue, 0.0);
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testIsoSpeedBadAttr() {
        FloatAttribute attr =
                StandardAttributes.getIsoSpeedAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testIsoSpeedBadValue() {
        float value = StandardAttributes.getIsoSpeed(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // EnvMap
    //=========================================================================
   
    @Test
    public void testEnvMap() {
        System.out.println("envMap");
        assertFalse(StandardAttributes.hasEnvMap(header));
        
        for (EnvMap value : EnvMap.values()) {
            StandardAttributes.addEnvMap(header, value);
            assertTrue(StandardAttributes.hasEnvMap(header));
            EnvMapAttribute attr =
                    StandardAttributes.getEnvMapAttribute(header);
            assertNotNull(attr);
            assertEquals(value, attr.getValue());
            // enums are immutable
            assertSame(value, attr.getValue());

            EnvMap hValue = StandardAttributes.getEnvMap(header);
            assertNotNull(hValue);
            assertEquals(value, hValue);
            // enums are immutable
            assertSame(value, hValue);
            assertSame(hValue, attr.getValue());

            checkSerialization();
        }
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testEnvMapBadAttr() {
        EnvMapAttribute attr =
                StandardAttributes.getEnvMapAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testEnvMapBadValue() {
        EnvMap value = StandardAttributes.getEnvMap(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // KeyCode
    //=========================================================================
   
    @Test
    public void testKeyCode() {
        System.out.println("keyCode");
        assertFalse(StandardAttributes.hasKeyCode(header));
        
        final KeyCode value = new KeyCode();
        value.filmMfcCode   =   10;
        value.filmType      =   20;
        value.prefix        = 1234;
        value.count         =    7;
        value.perfOffset    =  117;
        value.perfsPerFrame =    4;
        value.perfsPerCount =   64;
        
        StandardAttributes.addKeyCode(header, value);
        assertTrue(StandardAttributes.hasKeyCode(header));
        KeyCodeAttribute attr =
                StandardAttributes.getKeyCodeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        KeyCode hValue = StandardAttributes.getKeyCode(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.filmMfcCode = 31;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testKeyCodeBadAttr() {
        KeyCodeAttribute attr =
                StandardAttributes.getKeyCodeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testKeyCodeBadValue() {
        KeyCode value = StandardAttributes.getKeyCode(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // TimeCode
    //=========================================================================
   
    @Test
    public void testTimeCode() {
        System.out.println("timeCode");
        assertFalse(StandardAttributes.hasTimeCode(header));
        
        final TimeCode value = new TimeCode();
        value.setHours(10);
        value.setMinutes(53);
        value.setSeconds(26);
        value.setColorFrame(true);
        value.setFieldPhase(true);
        
        StandardAttributes.addTimeCode(header, value);
        assertTrue(StandardAttributes.hasTimeCode(header));
        TimeCodeAttribute attr =
                StandardAttributes.getTimeCodeAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        TimeCode hValue = StandardAttributes.getTimeCode(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.setFieldPhase(false);
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testTimeCodeBadAttr() {
        TimeCodeAttribute attr =
                StandardAttributes.getTimeCodeAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testTimeCodeBadValue() {
        TimeCode value = StandardAttributes.getTimeCode(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // Wrapmodes
    //=========================================================================
   
    @Test
    public void testWrapmodes() {
        System.out.println("wrapmodes");
        assertFalse(StandardAttributes.hasWrapmodes(header));
        
        final String value = "clamp, mirror ";
        
        StandardAttributes.addWrapmodes(header, value);
        assertTrue(StandardAttributes.hasWrapmodes(header));
        StringAttribute attr =
                StandardAttributes.getWrapmodesAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        // String are immutable
        assertSame(value, attr.getValue());
        
        String hValue = StandardAttributes.getWrapmodes(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        // String are immutable
        assertSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWrapmodesBadAttr() {
        StringAttribute attr =
                StandardAttributes.getWrapmodesAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWrapmodesBadValue() {
        String value = StandardAttributes.getWrapmodes(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // FramesPerSecond
    //=========================================================================
   
    @Test
    public void testFramesPerSecond() {
        System.out.println("framesPerSecond");
        assertFalse(StandardAttributes.hasFramesPerSecond(header));
        
        final Rational value = new Rational(24000, 1001);
        
        StandardAttributes.addFramesPerSecond(header, value);
        assertTrue(StandardAttributes.hasFramesPerSecond(header));
        RationalAttribute attr =
                StandardAttributes.getFramesPerSecondAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        Rational hValue = StandardAttributes.getFramesPerSecond(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.n = 30000;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testFramesPerSecondBadAttr() {
        RationalAttribute attr =
                StandardAttributes.getFramesPerSecondAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testFramesPerSecondBadValue() {
        Rational value = StandardAttributes.getFramesPerSecond(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // MultiView
    //=========================================================================
   
    @Test
    public void testMultiView() {
        System.out.println("multiView");
        assertFalse(StandardAttributes.hasMultiView(header));
        
        final List<String> value = Arrays.asList("Fie", "foh", "fum");
        
        StandardAttributes.addMultiView(header, value);
        assertTrue(StandardAttributes.hasMultiView(header));
        StringVectorAttribute attr =
                StandardAttributes.getMultiViewAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        List<String> hValue = StandardAttributes.getMultiView(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.set(0, "Fee");
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testMultiViewBadAttr() {
        StringVectorAttribute attr =
                StandardAttributes.getMultiViewAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testMultiViewBadValue() {
        List<String> value = StandardAttributes.getMultiView(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // WorldToCamera
    //=========================================================================
   
    @Test
    public void testWorldToCamera() {
        System.out.println("worldToCamera");
        assertFalse(StandardAttributes.hasWorldToCamera(header));
        
        final Matrix44<Float> value = new Matrix44<>(
                0.13547986f, 0.55495986f, 0.05334814f, 0.02035098f,
                0.69063298f, 0.58451742f, 0.00218974f, 0.77649942f,
                0.90180849f, 0.74402654f, 0.25503478f, 0.49102725f,
                0.47537465f, 0.40408873f, 0.79995949f, 0.33431727f);
        
        StandardAttributes.addWorldToCamera(header, value);
        assertTrue(StandardAttributes.hasWorldToCamera(header));
        M44fAttribute attr =
                StandardAttributes.getWorldToCameraAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        Matrix44<Float> hValue = StandardAttributes.getWorldToCamera(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.m03 *= 2.0f;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWorldToCameraBadAttr() {
        M44fAttribute attr =
                StandardAttributes.getWorldToCameraAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWorldToCameraBadValue() {
        Matrix44<Float> value = StandardAttributes.getWorldToCamera(header);
        fail("Got a non-existent value: " + value);
    }
    
    
    //=========================================================================
    // WorldToNDC
    //=========================================================================
   
    @Test
    public void testWorldToNDC() {
        System.out.println("worldToNDC");
        assertFalse(StandardAttributes.hasWorldToNDC(header));
        
        final Matrix44<Float> value = new Matrix44<>(
                0.73541952f, 0.77949297f, 0.50502189f, 0.43465520f,
                0.76939525f, 0.48426614f, 0.64031730f, 0.08019154f,
                0.86407300f, 0.39611682f, 0.40407443f, 0.72930679f,
                0.66669436f, 0.38090081f, 0.30044639f, 0.11202743f);
        
        StandardAttributes.addWorldToNDC(header, value);
        assertTrue(StandardAttributes.hasWorldToNDC(header));
        M44fAttribute attr =
                StandardAttributes.getWorldToNDCAttribute(header);
        assertNotNull(attr);
        assertEquals(value, attr.getValue());
        assertNotSame(value, attr.getValue());
        
        Matrix44<Float> hValue = StandardAttributes.getWorldToNDC(header);
        assertNotNull(hValue);
        assertEquals(value, hValue);
        assertNotSame(value, hValue);
        assertSame(hValue, attr.getValue());
        
        value.m03 *= 2.0f;
        assertFalse(value.equals(hValue));
        
        checkSerialization();
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWorldToNDCBadAttr() {
        M44fAttribute attr =
                StandardAttributes.getWorldToNDCAttribute(header);
        fail("Got a non-existent attribute: " + attr);
    }
    
    @Test(expected=IllegalArgumentException.class)
    public void testWorldToNDCBadValue() {
        Matrix44<Float> value = StandardAttributes.getWorldToNDC(header);
        fail("Got a non-existent value: " + value);
    }
    
}

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

#include "EXRSimpleImage.h"
#include <edu_cornell_graphics_exr_EXRSimpleImage.h>

#include <half.h>
#include <Iex.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>

#include <IlmThread.h>
#include <IlmThreadPool.h>

#include <fstream>
#include <cassert>

#include "util.h"
#if !USE_JAVA_UTF8
  #include "UnicodeStream.h"
#endif


namespace
{

// Our favourite exceptions for this DLL
const char *EXR_EXCEPTION = "edu/cornell/graphics/exr/EXRIOException";

} // namespace


// Initialize Attributes' static data
jfieldID Attributes::ownerID = NULL;
jfieldID Attributes::commentsID = NULL;
jfieldID Attributes::capDateID = NULL;
jfieldID Attributes::utcOffsetID = NULL;
jmethodID Attributes::constructorID = NULL;
bool Attributes::isCacheUpdated = false;


// Initial values for OpenEXRTo static data
jfieldID OpenEXRTo::attribID = NULL;
jfieldID OpenEXRTo::widthID = NULL;
jfieldID OpenEXRTo::heightID = NULL;
jfieldID OpenEXRTo::bufferID = NULL;
bool OpenEXRTo::isCacheUpdated = false;



// Updated save function with support for attributes
void saveToTransferObject(JNIEnv *env, jobject jTo,
      const Imf::Header &header,
      const Imf::Array2D<Imf::Rgba> &halfPixels,
      const int width, const int height, const int numChannels)
{
    assert(numChannels == 3 || numChannels == 4);
    OpenEXRTo to(jTo);

    // Start with the basics: width & height
    to.setWidth(env, width);
    to.setHeight(env, height);

    // Conversion from half to float
    const jsize numElements = width*height*numChannels;
    jfloatArray jbuffer = env->NewFloatArray(numElements);
    if (jbuffer == NULL) { return; /* exception thrown */ }

    // This method has greater chances to get the actual array data without extra copies
    jboolean isBufferCopy;
    float * pixels = (float*)env->GetPrimitiveArrayCritical(jbuffer, &isBufferCopy);

    const Imf::Rgba * halfPixelsPtr = &halfPixels[0][0];

    if (numChannels == 3) {

        for(int i = 0; i < width*height; ++i) {
            const int base = i*3;
            const Imf::Rgba & px = halfPixelsPtr[i];
            pixels[base]   = px.r;
            pixels[base+1] = px.g;
            pixels[base+2] = px.b;
        }

    }
    else if (numChannels == 4) {

        for(int i = 0; i < width*height; ++i) {
            const int base = i*4;
            const Imf::Rgba & px = halfPixelsPtr[i];
            pixels[base]   = px.r;
            pixels[base+1] = px.g;
            pixels[base+2] = px.b;
            pixels[base+3] = px.a;
        }
    }
    env->ReleasePrimitiveArrayCritical(jbuffer, pixels, 0);
    to.setBuffer(env, jbuffer);

    // Gets which attributes are in the header. If none is set
    // don't create/set the attributes object.
    if (Imf::hasComments(header) || Imf::hasOwner(header) || 
        Imf::hasCapDate(header)  || Imf::hasUtcOffset(header))
    {
        Attributes attrib(env, header);
        to.setAttributes(env, attrib);
    }
}



void Attributes::setStringField(JNIEnv *env, jfieldID fid, const char * val){
    assert(instance != NULL);
    jstring jstr = env->NewStringUTF(val);
    if (jstr == NULL) {
        throw JavaExc("JVM constructor returned null");
    }
    env->SetObjectField(instance, fid, jstr);
    if (env->ExceptionCheck()) {
        env->DeleteLocalRef(jstr);
        throw JavaExc("Java exception occurred.");
    }
    env->DeleteLocalRef(jstr);
}

void Attributes::setStringField(JNIEnv *env, jfieldID fid, const std::string & val) {
    setStringField(env, fid, val.c_str());
}

void Attributes::setFloatField(JNIEnv *env, jfieldID fid, jfloat val) {
    assert(instance != NULL);
    env->SetFloatField(instance, fid, val);
    if (env->ExceptionCheck()) {
        throw JavaExc("Java exception occurred.");
    }
}

void Attributes::setStringAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
    void (*attribMethod)(Imf::Header &, const std::string &) )
{
    assert(attribMethod != NULL);
    jstring jstr = (jstring)env->GetObjectField(instance, fid);
    if (jstr != NULL) {
        const char * str = env->GetStringUTFChars(jstr, NULL);
        if (str == NULL) {
            env->ReleaseStringUTFChars(jstr, str);
            throw JavaExc("Unexpected NULL String.");
        }
        (*attribMethod)(header, str);
        env->ReleaseStringUTFChars(jstr, str);
    }
}

void Attributes::setFloatAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
    void (*attribMethod)(Imf::Header &, const float &) ) 
{
    assert(attribMethod != NULL);
    union {jfloat val; unsigned int bits;};
    val = env->GetFloatField(instance, fid);
    // Add if the value is not NaN nor INFINITY
    if (bits != 0x7fc00000 && bits != 0x7f800000 && bits != 0xff800000) {
        (*attribMethod)(header, val);
    }
}

void Attributes::initCache(JNIEnv *env) 
{
    jclass attribClass = env->FindClass("edu/cornell/graphics/exr/Attributes");
    if (attribClass == NULL) { throw JavaExc("attribClass NULL"); }

    ownerID = env->GetFieldID(attribClass, "owner", "Ljava/lang/String;");
    if (ownerID == NULL) { 
        throw JavaExc("ownerID NULL"); 
    }

    commentsID = env->GetFieldID(attribClass, "comments", "Ljava/lang/String;");
    if (commentsID == NULL) { 
        throw JavaExc("commentsID NULL"); 
    }

    capDateID = env->GetFieldID(attribClass, "capDate", "Ljava/lang/String;");
    if (capDateID == NULL) { 
        throw JavaExc("capDateID NULL"); 
    }

    utcOffsetID = env->GetFieldID(attribClass, "utcOffset", "F");
    if (utcOffsetID == NULL) { 
        throw JavaExc("utcOffsetID NULL"); 
    }

    // We'll use the explicit boolean constructor
    constructorID = env->GetMethodID(attribClass, "<init>", "(Z)V");
    if (constructorID == NULL) { 
        throw JavaExc("constructorID NULL"); 
    }

    env->DeleteLocalRef(attribClass);
    isCacheUpdated = true;
}

Attributes::Attributes(JNIEnv *env, const Imf::Header & header) : instance(NULL) 
{
    assert(isCacheUpdated);

    // Gets which attributes are in the header. If none is set
    // don't create the attributes object.
    const bool hasComments  = Imf::hasComments(header);
    const bool hasOwner     = Imf::hasOwner(header);
    const bool hasCapDate   = Imf::hasCapDate(header);
    const bool hasUtcOffset = Imf::hasUtcOffset(header);

    // Note that it's not possible to cache the class, if we do we would
    // get an instance of InvocationTargetException or other bizarre things instead!!
    jclass attribClass = env->FindClass("edu/cornell/graphics/exr/Attributes");
    if (attribClass == NULL) { throw JavaExc("attribClass NULL"); }

    // Don't initialize anything: leave all the defaults
    instance = env->NewObject(attribClass, constructorID, JNI_FALSE);
    if (env->ExceptionCheck()) {
        env->DeleteLocalRef(attribClass);
        throw JavaExc("Java exception occurred.");
    }

    // Sets whatever fields are there
    try {
        if (hasComments) {
            setStringField(env, commentsID, Imf::comments(header) );
        }
        if (hasOwner) {
            setStringField(env, ownerID, Imf::owner(header) );
        }
        if (hasCapDate) {
            setStringField(env, capDateID, Imf::capDate(header) );
        }
        if (hasUtcOffset) {
            setFloatField(env, utcOffsetID, Imf::utcOffset(header) );
        }
        else if (hasCapDate) {
            setFloatField(env, utcOffsetID, 0.0f );
        }
    }
    catch(std::exception &e) {
        env->DeleteLocalRef(attribClass);
        throw e;
    }

    env->DeleteLocalRef(attribClass);
}


Attributes::Attributes(jobject attrib) : instance(attrib) {
    assert(isCacheUpdated);
}


// Adds the attributes to the header
void Attributes::setHeaderAttribs(JNIEnv *env, Imf::Header & header)
{
    assert(isCacheUpdated);
    if (instance == NULL) { return; }

    // Sets the attributes in the header for the valid fields
    setStringAttrib(env, header, ownerID,     &Imf::addOwner);
    setStringAttrib(env, header, commentsID,  &Imf::addComments);
    setStringAttrib(env, header, capDateID,   &Imf::addCapDate);
    setFloatAttrib( env, header, utcOffsetID, &Imf::addUtcOffset);
}



OpenEXRTo::OpenEXRTo(jobject obj) : instance(obj) {
    assert(instance != NULL);
}

void OpenEXRTo::setWidth(JNIEnv *env, int width) {
    assert(isCacheUpdated);
    env->SetIntField(instance, widthID, width);
    if (env->ExceptionCheck()) {
        throw JavaExc("Java exception occurred.");
    }
}

void OpenEXRTo::setHeight(JNIEnv *env, int height) {
    assert(isCacheUpdated);
    env->SetIntField(instance, heightID, height);
    if (env->ExceptionCheck()) {
        throw JavaExc("Java exception occurred.");
    }
}

void OpenEXRTo::setAttributes(JNIEnv *env, const Attributes & attrib) {
    assert(isCacheUpdated);
    env->SetObjectField(instance, attribID, attrib.getInstance());
    if (env->ExceptionCheck()) {
        throw JavaExc("Java exception occurred.");
    }
}

void OpenEXRTo::setBuffer(JNIEnv *env, jobject buffer) {
    assert(isCacheUpdated);
    env->SetObjectField(instance, bufferID, buffer);
    if (env->ExceptionCheck()) {
        throw JavaExc("Java exception occurred.");
    }
}

void OpenEXRTo::initCache(JNIEnv *env)
{
    jclass toClass = env->FindClass("edu/cornell/graphics/exr/EXRSimpleImage$OpenEXRTo");
    if (toClass == NULL) { throw JavaExc("toClass NULL"); }

    attribID = env->GetFieldID(toClass, "attrib", "Ledu/cornell/graphics/exr/Attributes;");
    if (attribID == NULL) { 
        throw JavaExc("attribID NULL"); 
    }

    widthID = env->GetFieldID(toClass, "width", "I");
    if (widthID == NULL) { 
        throw JavaExc("widthID NULL"); 
    }

    heightID = env->GetFieldID(toClass, "height", "I");
    if (heightID == NULL) { 
        throw JavaExc("heightID NULL"); 
    }

    bufferID = env->GetFieldID(toClass, "buffer", "[F");
    if (bufferID == NULL) { 
        throw JavaExc("bufferID NULL"); 
    }

    env->DeleteLocalRef(toClass);
    isCacheUpdated = true;
}



// Such a simple method, but will save painful debugging
jlong Java_edu_cornell_graphics_exr_EXRSimpleImage_getNativeVersion
    (JNIEnv *, jclass)
{
#if defined(__GNUC__) || defined(__clang__)
    return __extension__ edu_cornell_graphics_exr_EXRSimpleImage_serialVersionUID;
#else
    return edu_cornell_graphics_exr_EXRSimpleImage_serialVersionUID;
#endif
}



// Sets the number of global working threads, or throws a nasty exception
void Java_edu_cornell_graphics_exr_EXRSimpleImage_setNumWorkingThreads
    (JNIEnv *env, jclass, jint numThreads)
{
    assert(numThreads >= 0);
    if (IlmThread::supportsThreads()) {

        IlmThread::ThreadPool::globalThreadPool().setNumThreads(numThreads);
    } else {

        JNU_ThrowByName(env,
            "java/lang/UnsupportedOperationException",
            "The IlmThread library doesn't support threads.");
    }
}



// Initializes the type caches
void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_initCache
    (JNIEnv *env, jclass)
{
    try {
        Attributes::initCache(env);
        OpenEXRTo::initCache(env);
    }
    catch(std::exception & e) {
        if (! env->ExceptionCheck()) {
            JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
        }
    }
}



void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_read
    (JNIEnv *env, jclass, jobject jexrTo, jint numChannels, jstring jfilename)
{
    try {
        if (jfilename == NULL) {
            throw JavaExc("Null filename.");
        }

#if USE_JAVA_UTF8
        JNIUTFString filename(env, jfilename);
        Imf::RgbaInputFile file(filename);
#else
        JNIString filename(env, jfilename);
        UnicodeIFStream imfIs(filename, filename.len());
        Imf::RgbaInputFile file(imfIs);
#endif
        Imath::Box2i dw = file.dataWindow();
        const int width  = dw.max.x - dw.min.x + 1;
        const int height = dw.max.y - dw.min.y + 1;

        // Memory for reading the all pixels
        Imf::Array2D<Imf::Rgba> halfPixels(width, height);
        
        // Read all the pixels from the image
        file.setFrameBuffer (&halfPixels[0][0], 1, width);
        file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);

        // And saves that into the transfer object
        saveToTransferObject(env, jexrTo, file.header(), 
            halfPixels, width, height, numChannels);
    }
    catch (std::exception &e) {
        if (! env->ExceptionCheck() ) {
            // Something ugly has happened, so we throw that exception to Java
            // ... unless something so bad occured that an exception is already raised!
            JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
        }
    }

}



void Java_edu_cornell_graphics_exr_EXRSimpleImage_write
    (JNIEnv *env, jclass, jstring jfilename, jfloatArray jpixelBuffer, 
     jint width, jint height,
     jint numChannels, jobject jattrib, jint compressionFlag)
{
    assert(numChannels == 3 || numChannels == 4);

    try {
        if (jfilename == NULL) {
            throw JavaExc("Null filename.");
        }
        
        // Converted version of the memory
        Imf::Array2D<Imf::Rgba> halfPixels(width, height);

        // Convert the pixels according to the channel configuration
        jboolean isBufferCopy;
        const float *pixels = (const float*)env->GetPrimitiveArrayCritical(
            jpixelBuffer, &isBufferCopy);

        const int numPixels = width*height;
        Imf::Rgba * halfPixelsPtr = &halfPixels[0][0];
        if (numChannels == 3) {
            for(int i = 0; i < numPixels; ++i) {
                const int base = i*3;
                halfPixelsPtr[i].r = pixels[base];
                halfPixelsPtr[i].g = pixels[base+1];
                halfPixelsPtr[i].b = pixels[base+2];
            }
        }
        else if (numChannels == 4) {
            for(int i = 0; i < numPixels; ++i) {
                const int base = i*4;
                halfPixelsPtr[i].r = pixels[base];
                halfPixelsPtr[i].g = pixels[base+1];
                halfPixelsPtr[i].b = pixels[base+2];
                halfPixelsPtr[i].a = pixels[base+3];
            }
        }
        
        const Imf::RgbaChannels channels = numChannels == 3 ?
            Imf::WRITE_RGB : Imf::WRITE_RGBA;
        env->ReleasePrimitiveArrayCritical(jpixelBuffer,
            const_cast<float*>(pixels), 0);

        Imf::Header hd(width, height,
               /* pixelAspectRatio */   1,
               /* screenWindowCenter */ Imath::V2f(0,0),
               /* screenWindowWidth */  1,
               /* lineOrder */          Imf::INCREASING_Y,
               /* compression) */       static_cast<Imf::Compression>(compressionFlag) );

        if(jattrib != NULL) {
            Attributes attrib(jattrib);
            attrib.setHeaderAttribs(env, hd);
        }

        // Actually write the file
#if USE_JAVA_UTF8
        JNIUTFString filename(env, jfilename);
        Imf::RgbaOutputFile file(filename, hd, channels);
#else
        JNIString filename(env, jfilename);
        UnicodeOFStream omfOs(filename, filename.len());
        Imf::RgbaOutputFile file(omfOs, hd, channels);
#endif
        file.setFrameBuffer(&halfPixels[0][0], 1, width);
        file.writePixels(height);

    }
    catch (std::exception &e) {
        // Something ugly has happened, so we throw that exception to Java
        JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
    }
}

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

#include <edu_cornell_graphics_exr_NativeFrameBuffer.h>
#include "util.h"

#include <ImfFrameBuffer.h>

#include <memory>


namespace
{

class JVMData
{
public:
    JVMData(JNIEnv* env);
    Imf::Slice getSlice(JNIEnv* env, jobject jslice) const;

private:
    struct {
        jclass clazz;
        jmethodID ordinal;
    } m_pixelType;

    struct {
        jclass clazz;
        jfieldID type;
        jfieldID buffer;
        jfieldID baseOffset;
        jfieldID xStride;
        jfieldID yStride;
        jfieldID xSampling;
        jfieldID ySampling;
        jfieldID fillValue;
        jfieldID xTileCoords;
        jfieldID yTileCoords;
    } m_slice;
};

JVMData::JVMData(JNIEnv* envPtr)
{
    JNIEnvHelper env(envPtr);

    m_pixelType.clazz   = env.findClassGlobalRef("edu/cornell/graphics/exr/PixelType");
    m_pixelType.ordinal = env.getMethodID(m_pixelType.clazz, "ordinal", "()I");

    m_slice.clazz       = env.findClassGlobalRef("edu/cornell/graphics/exr/Slice");
    m_slice.type        = env.getFieldID(m_slice.clazz, "type",
        "Ledu/cornell/graphics/exr/PixelType;");
    m_slice.buffer      = env.getFieldID(m_slice.clazz, "buffer",
        "Ljava/nio/ByteBuffer;");
    m_slice.baseOffset  = env.getFieldID(m_slice.clazz, "baseOffset",  "I");
    m_slice.xStride     = env.getFieldID(m_slice.clazz, "xStride",     "I");
    m_slice.yStride     = env.getFieldID(m_slice.clazz, "yStride",     "I");
    m_slice.xSampling   = env.getFieldID(m_slice.clazz, "xSampling",   "I");
    m_slice.ySampling   = env.getFieldID(m_slice.clazz, "ySampling",   "I");
    m_slice.fillValue   = env.getFieldID(m_slice.clazz, "fillValue",   "D");
    m_slice.xTileCoords = env.getFieldID(m_slice.clazz, "xTileCoords", "Z");
    m_slice.yTileCoords = env.getFieldID(m_slice.clazz, "yTileCoords", "Z");
}

/// <summary>
/// Creates an Imf::Slice instance from its Java counterside. After finishing
/// it deletes the local reference to the Java slice object.
/// </summary>
Imf::Slice JVMData::getSlice(JNIEnv* env, jobject jslice) const
{
    if (env->IsInstanceOf(jslice, m_slice.clazz) != JNI_TRUE) {
        throw JavaExc("Not a Slice instance");
    }

    Imf::Slice slice;

    // Pixel type. We assume that the ordinals match the native code.
    {
        jobject jtype = env->GetObjectField(jslice, m_slice.type);
        if (env->IsInstanceOf(jtype, m_pixelType.clazz) != JNI_TRUE) {
            throw JavaExc("Not a PixelType instance");
        }
        int baseType = env->CallIntMethod(jtype, m_pixelType.ordinal);
        switch (baseType) {
        case Imf::UINT:
        case Imf::HALF:
        case Imf::FLOAT:
            slice.type = static_cast<Imf::PixelType>(baseType);
            break;
        default:
            throw JavaExc("Invalid PixelType ordinal");
        }
        env->DeleteLocalRef(jtype);
    }
    
    // We assume that the buffer is a direct buffer.
    {
        jint baseOffset = env->GetIntField(jslice, m_slice.baseOffset);
        jobject jbuffer = env->GetObjectField(jslice, m_slice.buffer);
        slice.base = static_cast<char*>(env->GetDirectBufferAddress(jbuffer));
        env->DeleteLocalRef(jbuffer);
        if (slice.base == nullptr) {
            throw JavaExc("Invalid slice buffer");
        }
        slice.base += baseOffset;
    }

    slice.xStride     =  env->GetIntField(jslice, m_slice.xStride);
    slice.yStride     =  env->GetIntField(jslice, m_slice.yStride);
    slice.xSampling   =  env->GetIntField(jslice, m_slice.xSampling);
    slice.ySampling   =  env->GetIntField(jslice, m_slice.ySampling);
    slice.fillValue   =  env->GetDoubleField(jslice, m_slice.fillValue);
    slice.xTileCoords =
        env->GetBooleanField(jslice, m_slice.xTileCoords) != JNI_FALSE;
    slice.yTileCoords =
        env->GetBooleanField(jslice, m_slice.yTileCoords) != JNI_FALSE;

    env->DeleteLocalRef(jslice);
    return slice;
}


// Actual instance of the JVM Data cache
std::auto_ptr<JVMData> jvmData;

} // namespace



void Java_edu_cornell_graphics_exr_NativeFrameBuffer_initNativeCache(
    JNIEnv* env, jclass)
{
    JVMData* const data = safeCall(env, [](JNIEnv* env) {
        return new JVMData(env);
    }, static_cast<JVMData*>(nullptr));
    jvmData.reset(data);
}



jlong Java_edu_cornell_graphics_exr_NativeFrameBuffer_newNativeFrameBuffer(
    JNIEnv* env, jclass, jint count, jobjectArray names, jobjectArray slices)
{
    using Imf::FrameBuffer;
    FrameBuffer* const frameBuffer = safeCall(env,
        [count, names, slices](JNIEnv* env) -> FrameBuffer* {
        FrameBuffer* frameBuffer = new FrameBuffer;
        for (jint i = 0; i < count; ++i) {
            Imf::Slice slice(jvmData->getSlice(env,
                env->GetObjectArrayElement(slices, i)));
            JNIUTFString name(env, static_cast<jstring>(
                env->GetObjectArrayElement(names, i)));
            frameBuffer->insert(name, slice);
        }
        return frameBuffer;
    }, static_cast<FrameBuffer*>(nullptr));
    return reinterpret_cast<jlong>(frameBuffer);
}



void Java_edu_cornell_graphics_exr_NativeFrameBuffer_deleteNativeHandle(
    JNIEnv* env, jclass, jlong nativeFramBufferPtr)
{
    using Imf::FrameBuffer;
    safeCall(env, [nativeFramBufferPtr] (JNIEnv* env) {
        if (nativeFramBufferPtr == 0) {
            throw Iex::ArgExc("null Imf::FrameBuffer pointer");
        }
        // We assume that nativeFramBufferPtr actually contains a valid
        // instance, otherwise there will be an access violation exception
        FrameBuffer* to = reinterpret_cast<FrameBuffer*>(nativeFramBufferPtr);
        delete to;
    });
}

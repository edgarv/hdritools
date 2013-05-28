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

#include <edu_cornell_graphics_exr_EXRInputFile.h>

#include "EXRJavaStream.h"
#include "StdSStream.h"
#include "util.h"
#if !USE_JAVA_UTF8
# include "UnicodeStream.h"
#endif

#include <ImfInputFile.h>
#include <ImfVersion.h>

#include <cassert>
#include <memory>


namespace
{
struct EXRInputFileTO
{
    // The order is important! The stream has to be destructed after the file
    std::unique_ptr<Imf::IStream>   m_stream;
    std::unique_ptr<Imf::InputFile> m_inputFile;
};



class JVMData {
public:
    JVMData(JNIEnv* env);

    Imf::Slice getSlice(JNIEnv* env, jobject jslice) const;

private:
    JavaVM* m_jvm;

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
    
    m_jvm = env.getJavaVM();

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
std::unique_ptr<JVMData> jvmData;


// C++11 Magic to write less code, for functions which return a value
template <class MethodImpl, typename T>
T safeCall(JNIEnv* env, const MethodImpl& method, const T& defaultValue)
{
    try {
        T retVal = method(env);
        return retVal;
    } catch (JavaExc& ex0) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            std::ostringstream os;
            os << ex0.what();
            if (!ex0.stackTrace().empty()) {
                os << std::endl << "Stack trace:" 
                   << std::endl << ex0.stackTrace();
            }
            JNU_ThrowRuntimeException(env, os.str());
        }
    } catch (Iex::BaseExc& ex1) {
        std::ostringstream os;
        os << ex1.what();
        if (!ex1.stackTrace().empty()) {
            os << std::endl << "Stack trace:" << std::endl << ex1.stackTrace();
        }
        JNU_ThrowRuntimeException(env, os.str());
    } catch (std::exception& ex2) {
        JNU_ThrowRuntimeException(env, ex2.what());
    } catch (...) {
        JNU_ThrowRuntimeException(env, "Unknown native error!");
    }

    return defaultValue;
}

/// C++11 Magic to write less code, for void functions
template <class MethodImpl>
void safeCall(JNIEnv* env, const MethodImpl& method)
{
    try {
        method(env);
    } catch (JavaExc& ex0) {
        if (env->ExceptionCheck() == JNI_FALSE) {
            std::ostringstream os;
            os << ex0.what();
            if (!ex0.stackTrace().empty()) {
                os << std::endl << "Stack trace:" 
                   << std::endl << ex0.stackTrace();
            }
            JNU_ThrowRuntimeException(env, os.str());
        }
    } catch (Iex::BaseExc& ex1) {
        std::ostringstream os;
        os << ex1.what();
        if (!ex1.stackTrace().empty()) {
            os << std::endl << "Stack trace:" << std::endl << ex1.stackTrace();
        }
        JNU_ThrowRuntimeException(env, os.str());
    } catch (std::exception& ex2) {
        JNU_ThrowRuntimeException(env, ex2.what());
    } catch (...) {
        JNU_ThrowRuntimeException(env, "Unknown native error!");
    }
}

} // namespace



void Java_edu_cornell_graphics_exr_EXRInputFile_initNativeCache(
    JNIEnv* env, jclass)
{
    JVMData* const data = safeCall(env, [](JNIEnv* env) {
        return new JVMData(env);
    }, static_cast<JVMData*>(nullptr));
    jvmData.reset(data);
}



jlong Java_edu_cornell_graphics_exr_EXRInputFile_getNativeInputFile(
    JNIEnv* env, jclass, jobject is, jstring jfilename, jint numThreads)
{
    using Imf::InputFile;

    auto impl = [is, jfilename, numThreads] (JNIEnv* env) -> EXRInputFileTO* {
       EXRInputFileTO* to;
       if (!(!is ^ !jfilename)) {
            throw JavaExc(is == nullptr ?
                "both stream are filename were provided" :
                "both stream are filename are null");
            return nullptr;
        } else  if (is != nullptr) {
            to = new EXRInputFileTO;
            to->m_stream.reset(new EXRJavaInputStream(env, is));
            to->m_inputFile.reset(new InputFile(*to->m_stream, numThreads));
        } else {
            assert(jfilename != nullptr);
            to = new EXRInputFileTO;
#if USE_JAVA_UTF8
            JNIUTFString filename(env, jfilename);
            to->m_inputFile.reset(new InputFile(filename, numThreads));
#else
            JNIString filename(env, jfilename);
            to->m_stream.reset(new UnicodeIFStream(filename, filename.len()));
            to->m_inputFile.reset(new InputFile(*to->m_stream, numThreads));
#endif
        }
        return to;
    };

    auto retVal = safeCall(env, impl, static_cast<EXRInputFileTO*>(nullptr));
    return reinterpret_cast<jlong>(retVal);
}



void Java_edu_cornell_graphics_exr_EXRInputFile_deleteNativeInputFile
    (JNIEnv *env, jclass, jlong nativePtr)
{
    safeCall(env, [nativePtr] (JNIEnv* env) {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        // At this point we assume that nativePtr actually contains a valid
        // instance, otherwise there will be an access violation exception
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        delete to;
    });
}



jint Java_edu_cornell_graphics_exr_EXRInputFile_getNativeVersion(
    JNIEnv* env, jclass, jlong nativePtr)
{
    int version = safeCall(env, [nativePtr] (JNIEnv* env) -> int {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        return to->m_inputFile->version();
    }, static_cast<int>(-1));
    return version;
}



jboolean Java_edu_cornell_graphics_exr_EXRInputFile_isNativeComplete(
    JNIEnv* env, jclass, jlong nativePtr)
{
    bool complete = safeCall(env, [nativePtr] (JNIEnv* env) -> bool {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        return to->m_inputFile->isComplete();
    }, false);
    return complete;
}



jbyteArray Java_edu_cornell_graphics_exr_EXRInputFile_getNativeHeaderBuffer(
    JNIEnv* env, jclass, jlong nativePtr)
{
    jbyteArray arr = safeCall(env, [nativePtr] (JNIEnv* env) -> jbyteArray {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }

        // Recreating the header on the Java side through JNI would be
        // extremely cumbersome. Instead we rely on the existing Java-based
        // reading code, effectively serializing the existing header
        StdOSStream os;
        bool isTiled = Imf::isTiled(to->m_inputFile->version());
        to->m_inputFile->header().writeTo(os, isTiled);
        const std::string headerData(os.str());

        // Copy into a Java-managed byte array, excluding the version
        // and magic numbers written by Imf::Header::writeTo
        const jsize len = static_cast<jsize>(headerData.size() - 8);
        if (len < 1) {
            throw JavaExc("Invalid header size");
        }
        jbyteArray jheaderArray = env->NewByteArray(len);
        if (jheaderArray == NULL) {
            throw JavaExc("Could not create the header byte array");
        }
        static_assert(sizeof(char) == sizeof(jbyte), "jbyte/char bad size");
        env->SetByteArrayRegion(jheaderArray, 0, len,
            reinterpret_cast<const jbyte*>(&headerData[8]));
        return jheaderArray;
    }, static_cast<jbyteArray>(NULL));
    return arr;
}



void Java_edu_cornell_graphics_exr_EXRInputFile_setNativeFrameBuffer(
    JNIEnv* env, jclass, jlong nativePtr, jint count,
    jobjectArray names, jobjectArray slices)
{
    safeCall(env, [nativePtr, count, names, slices](JNIEnv* env) {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }

        Imf::FrameBuffer frameBuffer;
        for (jint i = 0; i < count; ++i) {
            Imf::Slice slice(jvmData->getSlice(env,
                env->GetObjectArrayElement(slices, i)));
            JNIUTFString name(env, static_cast<jstring>(
                env->GetObjectArrayElement(names, i)));
            frameBuffer.insert(name, slice);
        }
        to->m_inputFile->setFrameBuffer(frameBuffer);
    });
}



void Java_edu_cornell_graphics_exr_EXRInputFile_readNativePixels(
    JNIEnv* env, jclass, jlong nativePtr, jint scanLine1, jint scanLine2)
{
    safeCall(env, [nativePtr, scanLine1, scanLine2](JNIEnv* env) {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        to->m_inputFile->readPixels(scanLine1, scanLine2);
    });
}

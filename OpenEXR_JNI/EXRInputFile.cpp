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
#if defined(OPENEXR_IMF_INTERNAL_NAMESPACE)
#define USE_OPENEXR_2 1
#include <ImfGenericOutputFile.h>
#endif


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



#if USE_OPENEXR_2
class DummyEXROutputFile : public OPENEXR_IMF_INTERNAL_NAMESPACE::GenericOutputFile {
public:
    DummyEXROutputFile(OPENEXR_IMF_INTERNAL_NAMESPACE::OStream& os,
        const OPENEXR_IMF_INTERNAL_NAMESPACE::Header& header) {
        writeMagicNumberAndVersionField(os, header);
    }
};
#else
class DummyEXROutputFile {
public:
    DummyEXROutputFile(Imf::OStream& os, const Imf::Header& header) {}
};
#endif



// Functor for EXRInputFile_getNativeInputFile
class Functor_getNativeInputFile
{
public:
    Functor_getNativeInputFile(jobject _is, jstring _jfilename,
        jint _numThreads) :
    is(_is), jfilename(_jfilename), numThreads(_numThreads) {}

    EXRInputFileTO* operator() (JNIEnv* env) const
    {
        using Imf::InputFile;
        EXRInputFileTO* to;
        if (!(!is ^ !jfilename)) {
            throw JavaExc(is != nullptr ?
                "both stream and filename were provided" :
                "both stream and filename are null");
            return nullptr;
        } else if (is != nullptr) {
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
    }

private:
    const jobject is;
    const jstring jfilename;
    const jint    numThreads;
};



// Functor for EXRInputFile_deleteNativeInputFile
class Functor_deleteNativeInputFile
{
public:
    Functor_deleteNativeInputFile(jlong _nativePtr) : nativePtr(_nativePtr) {}

    void operator() (JNIEnv* env) const
    {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        // At this point we assume that nativePtr actually contains a valid
        // instance, otherwise there will be an access violation exception
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        delete to;
    }
private:
    const jlong nativePtr;
};



// Functor for EXRInputFile_getNativeVersion
class Functor_getNativeVersion
{
public:
    Functor_getNativeVersion(jlong _nativePtr) : nativePtr(_nativePtr) {}

    int operator() (JNIEnv* env) const
    {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        return to->m_inputFile->version();
    }

private:
    const jlong nativePtr;
};



// Functor for EXRInputFile_isNativeComplete
class Functor_isNativeComplete
{
public:
    Functor_isNativeComplete(jlong _nativePtr) : nativePtr(_nativePtr) {}

    bool operator() (JNIEnv* env) const
    {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        return to->m_inputFile->isComplete();
    }

private:
    const jlong nativePtr;
};



// Functor for EXRInputFile_getNativeHeaderBuffer
class Functor_getNativeHeaderBuffer
{
public:
    Functor_getNativeHeaderBuffer(jlong _nativePtr) : nativePtr(_nativePtr) {}

    jbyteArray operator() (JNIEnv* env) const
    {
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
        // Before OpenEXR 2, writeTo wrote the magic number and the version,
        // later it was separated into two functions
        DummyEXROutputFile dummy(os, to->m_inputFile->header());
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
    }

private:
    const jlong nativePtr;
};



// Functor for EXRInputFile_setNativeFrameBuffer
class Functor_setNativeFrameBuffer
{
public:
    Functor_setNativeFrameBuffer(jlong _nativePtr, jlong _nativeFrameBufferPtr):
    nativePtr(_nativePtr), nativeFrameBufferPtr(_nativeFrameBufferPtr)
    {
        // empty
    }

    void operator() (JNIEnv* env) const
    {
        using Imf::FrameBuffer;
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        } else if (nativeFrameBufferPtr == 0) {
            throw Iex::ArgExc("null Imf::FrameBuffer pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(nativeFrameBufferPtr);
        to->m_inputFile->setFrameBuffer(*fb);
    }

private:
    const jlong nativePtr;
    const jlong nativeFrameBufferPtr;
};



// Functor for EXRInputFile_readNativePixels
class Functor_readNativePixels
{
public:
    Functor_readNativePixels(jlong _nativePtr,jint _scanLine1,jint _scanLine2) :
    nativePtr(_nativePtr), scanLine1(_scanLine1), scanLine2(_scanLine2) {}

    void operator() (JNIEnv* env) const
    {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXRInputFileTO pointer");
        }
        EXRInputFileTO* to = reinterpret_cast<EXRInputFileTO*>(nativePtr);
        if (to->m_inputFile == nullptr) {
            throw Iex::LogicExc("null input file");
        }
        to->m_inputFile->readPixels(scanLine1, scanLine2);
    }

private:
    const jlong nativePtr;
    const jint scanLine1;
    const jint scanLine2;
};

} // namespace



jlong JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_getNativeInputFile(
    JNIEnv* env, jclass, jobject is, jstring jfilename, jint numThreads)
{
    Functor_getNativeInputFile impl(is, jfilename, numThreads);
    auto retVal = safeCall(env, impl, static_cast<EXRInputFileTO*>(nullptr));
    return reinterpret_cast<jlong>(retVal);
}



void JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_deleteNativeInputFile
    (JNIEnv *env, jclass, jlong nativePtr)
{
    Functor_deleteNativeInputFile impl(nativePtr);
    safeCall(env, impl);
}



jint JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_getNativeVersion(
    JNIEnv* env, jclass, jlong nativePtr)
{
    Functor_getNativeVersion impl(nativePtr);
    int version = safeCall(env, impl, static_cast<int>(-1));
    return version;
}



jboolean JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_isNativeComplete(
    JNIEnv* env, jclass, jlong nativePtr)
{
    Functor_isNativeComplete impl(nativePtr);
    bool complete = safeCall(env, impl, false);
    return complete;
}



jbyteArray
JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_getNativeHeaderBuffer(
    JNIEnv* env, jclass, jlong nativePtr)
{
    Functor_getNativeHeaderBuffer impl(nativePtr);
    jbyteArray arr = safeCall(env, impl, static_cast<jbyteArray>(NULL));
    return arr;
}



void JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_setNativeFrameBuffer(
    JNIEnv* env, jclass, jlong nativePtr, jlong nativeFrameBufferPtr)
{
    Functor_setNativeFrameBuffer impl(nativePtr, nativeFrameBufferPtr);
    safeCall(env, impl);
}



void JNICALL Java_edu_cornell_graphics_exr_EXRInputFile_readNativePixels(
    JNIEnv* env, jclass, jlong nativePtr, jint scanLine1, jint scanLine2)
{
    Functor_readNativePixels impl(nativePtr, scanLine1, scanLine2);
    safeCall(env, impl);
}

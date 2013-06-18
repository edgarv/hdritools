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

#include <edu_cornell_graphics_exr_EXROutputFile.h>

#include "util.h"
#include "EXRJavaStream.h"
#include "StdSStream.h"
#if !USE_JAVA_UTF8
# include "UnicodeStream.h"
#endif

#include <ImfOutputFile.h>

#include <memory>
#include <cassert>

namespace
{
struct EXROutputFileTO
{
    // The order is important! The stream has to be destructed after the file
    std::unique_ptr<Imf::OStream>    m_stream;
    std::unique_ptr<Imf::OutputFile> m_outputFile;

    inline static EXROutputFileTO* get(jlong nativePtr) {
        if (nativePtr == 0) {
            throw Iex::ArgExc("null EXROutputFileTO pointer");
        }
        // At this point we assume that nativePtr actually contains a valid
        // instance, otherwise there will be an access violation exception
        EXROutputFileTO* to = reinterpret_cast<EXROutputFileTO*>(nativePtr);
        return to;
    }
};



// Functor for EXROutputFile_getNativeOutputFile
class Functor_getNativeOutputFile
{
public:
    Functor_getNativeOutputFile(jbyteArray _jHeaderBuffer,
        jint _jHeaderBufferLen,jobject _os,jstring _jfilename,jint _nThreads) :
    jHeaderBuffer(_jHeaderBuffer), jHeaderBufferLen(_jHeaderBufferLen),
    os(_os), jfilename(_jfilename), nThreads(_nThreads) {}

    EXROutputFileTO* operator() (JNIEnv* env) const
    {
        using Imf::OutputFile;
        using Imf::Header;
        if (!(!os ^ !jfilename)) {
            throw JavaExc(os != nullptr ?
                "both stream and filename were provided" :
                "both stream and filename are null");
            return nullptr;
        }

        void* const jPtr = env->GetPrimitiveArrayCritical(jHeaderBuffer, nullptr);
        const std::string hStr(static_cast<char*>(jPtr), jHeaderBufferLen); 
        env->ReleasePrimitiveArrayCritical(jHeaderBuffer, jPtr, JNI_ABORT);
        StdISStream isstream(hStr);

        Header header;
        int version;
        header.readFrom(isstream, version);

        EXROutputFileTO* const to = new EXROutputFileTO;
        if (os != nullptr) {
            to->m_stream.reset(new EXRJavaOutputStream(env, os));
            to->m_outputFile.reset(
                new OutputFile(*to->m_stream, header, nThreads));
        } else {
            assert(jfilename != nullptr);
#if USE_JAVA_UTF8
            JNIUTFString filename(env, jfilename);
            to->m_outputFile.reset(new OutputFile(filename, header, nThreads));
#else
            JNIString filename(env, jfilename);
            to->m_stream.reset(new UnicodeOFStream(filename, filename.len()));
            to->m_outputFile.reset(
                new OutputFile(*to->m_stream, header, nThreads));
#endif
        }
        assert(to->m_outputFile);
        return to;
    }

private:
    const jbyteArray jHeaderBuffer;
    const jint jHeaderBufferLen;
    const jobject os;
    const jstring jfilename;
    const jint nThreads;
};



// Functor for EXROutputFile_deleteNativeOutputFile
class Functor_deleteNativeOutputFile
{
public:
    Functor_deleteNativeOutputFile(jlong _nativePtr) : nativePtr(_nativePtr) {}

    void operator() (JNIEnv* env) const {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        delete to;
    }

private:
    const jlong nativePtr;
};



// Functor for EXROutputFile_setNativeFrameBuffer
class Functor_setNativeFrameBuffer
{
public:
    Functor_setNativeFrameBuffer(jlong _nativePtr, jlong _nativeFrameBufferPtr):
    nativePtr(_nativePtr), nativeFrameBufferPtr(_nativeFrameBufferPtr) {}

    void operator() (JNIEnv* env) const
    {
        using Imf::FrameBuffer;
        if (nativeFrameBufferPtr == 0) {
            throw Iex::ArgExc("null Imf::FrameBuffer pointer");
        }
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(nativeFrameBufferPtr);
        to->m_outputFile->setFrameBuffer(*fb);
    }

private:
    const jlong nativePtr;
    const jlong nativeFrameBufferPtr;
};



// Functor for EXROutputFile_currentNativeScanLine
class Functor_currentNativeScanLine
{
public:
    Functor_currentNativeScanLine(jlong _nativePtr) : nativePtr(_nativePtr) {}

    int operator() (JNIEnv* env) const {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        return to->m_outputFile->currentScanLine();
    }

private:
    const jlong nativePtr;
};



// Functor for EXROutputFile_writeNativePixels
class Functor_writeNativePixels
{
public:
    Functor_writeNativePixels(jlong _nativePtr, jint _numScanlines) :
    nativePtr(_nativePtr), numScanlines(_numScanlines) {}

    void operator() (JNIEnv* env) const {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        to->m_outputFile->writePixels(numScanlines);
    }

private:
    const jlong nativePtr;
    const jint numScanlines;
};

} // namespace



jlong JNICALL Java_edu_cornell_graphics_exr_EXROutputFile_getNativeOutputFile(
    JNIEnv* env, jclass, jbyteArray jHeaderBuffer, jint jHeaderBufferLen,
    jobject os, jstring jfilename, jint nThreads)
{
    Functor_getNativeOutputFile impl(jHeaderBuffer, jHeaderBufferLen,
        os, jfilename, nThreads);
    EXROutputFileTO* retVal = safeCall(env, impl,
        static_cast<EXROutputFileTO*>(nullptr));
    return reinterpret_cast<jlong>(retVal);
}



void JNICALL Java_edu_cornell_graphics_exr_EXROutputFile_deleteNativeOutputFile(
    JNIEnv* env, jclass, jlong nativePtr)
{
    Functor_deleteNativeOutputFile impl(nativePtr);
    safeCall(env, impl);
}



void JNICALL Java_edu_cornell_graphics_exr_EXROutputFile_setNativeFrameBuffer(
    JNIEnv* env, jclass, jlong nativePtr, jlong nativeFrameBufferPtr)
{
    Functor_setNativeFrameBuffer impl(nativePtr, nativeFrameBufferPtr);
    safeCall(env, impl);
}



jint JNICALL Java_edu_cornell_graphics_exr_EXROutputFile_currentNativeScanLine(
    JNIEnv* env, jclass, jlong nativePtr)
{
    Functor_currentNativeScanLine impl(nativePtr);
    int retVal = safeCall(env, impl, static_cast<int>(0));
    return retVal;
}



void JNICALL Java_edu_cornell_graphics_exr_EXROutputFile_writeNativePixels(
    JNIEnv* env, jclass, jlong nativePtr, jint numScanlines)
{
    Functor_writeNativePixels impl(nativePtr, numScanlines);
    safeCall(env, impl);
}

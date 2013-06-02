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

} // namespace



jlong Java_edu_cornell_graphics_exr_EXROutputFile_getNativeOutputFile(
    JNIEnv* env, jclass, jbyteArray jHeaderBuffer, jint jHeaderBufferLen,
    jobject os, jstring jfilename, jint nThreads)
{
    using Imf::OutputFile;
    using Imf::Header;
    EXROutputFileTO* retVal = safeCall(env,
        [jHeaderBuffer, jHeaderBufferLen, os, jfilename, nThreads] 
    (JNIEnv* env) -> EXROutputFileTO*
    {
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

    }, static_cast<EXROutputFileTO*>(nullptr));
    return reinterpret_cast<jlong>(retVal);
}



void Java_edu_cornell_graphics_exr_EXROutputFile_deleteNativeOutputFile(
    JNIEnv* env, jclass, jlong nativePtr)
{
    safeCall(env, [nativePtr] (JNIEnv* env) {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        delete to;
    });
}



void Java_edu_cornell_graphics_exr_EXROutputFile_setNativeFrameBuffer(
    JNIEnv* env, jclass, jlong nativePtr, jlong nativeFrameBufferPtr)
{
    using Imf::FrameBuffer;
    safeCall(env, [nativePtr, nativeFrameBufferPtr](JNIEnv* env) {
        if (nativeFrameBufferPtr == 0) {
            throw Iex::ArgExc("null Imf::FrameBuffer pointer");
        }
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(nativeFrameBufferPtr);
        to->m_outputFile->setFrameBuffer(*fb);
    });
}



jint Java_edu_cornell_graphics_exr_EXROutputFile_currentNativeScanLine(
    JNIEnv* env, jclass, jlong nativePtr)
{
    int retVal = safeCall(env, [nativePtr] (JNIEnv* env) -> int {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        return to->m_outputFile->currentScanLine();
    }, static_cast<int>(0));
    return retVal;
}



void Java_edu_cornell_graphics_exr_EXROutputFile_writeNativePixels(
    JNIEnv* env, jclass, jlong nativePtr, jint numScanlines)
{
    safeCall(env, [nativePtr, numScanlines](JNIEnv* env) {
        EXROutputFileTO* to = EXROutputFileTO::get(nativePtr);
        if (to->m_outputFile == nullptr) {
            throw Iex::LogicExc("null output file");
        }
        to->m_outputFile->writePixels(numScanlines);
    });
}

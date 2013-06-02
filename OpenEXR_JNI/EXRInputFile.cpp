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

} // namespace



jlong Java_edu_cornell_graphics_exr_EXRInputFile_getNativeInputFile(
    JNIEnv* env, jclass, jobject is, jstring jfilename, jint numThreads)
{
    using Imf::InputFile;

    auto impl = [is, jfilename, numThreads] (JNIEnv* env) -> EXRInputFileTO* {
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
    JNIEnv* env, jclass, jlong nativePtr, jlong nativeFrameBufferPtr)
{
    using Imf::FrameBuffer;
    safeCall(env, [nativePtr, nativeFrameBufferPtr](JNIEnv* env) {
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

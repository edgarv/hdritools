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

#include "EXRJavaStream.h"
#include "util.h"

#include <Iex.h>
#include <IlmThreadMutex.h>
#include <ImfInputFile.h>

#include <cassert>

using IlmThread::Mutex;
using IlmThread::Lock;


namespace
{
// Stored here to keep the main class header simpler
Mutex mutex;
}

volatile EXRJavaInputStream::JVMData* EXRJavaInputStream::jvmData = NULL;
std::unique_ptr<volatile EXRJavaOutputStream::JVMData>
EXRJavaOutputStream::jvmData;



EXRJavaInputStream::JVMData::JVMData(JNIEnv* env)
{
    assert(env != NULL);
    if (env->GetJavaVM(&jvm) != JNI_OK) {
        throw JavaExc("Could not get pointer to the Java VM");
    }
    clazz = env->FindClass("edu/cornell/graphics/exr/io/EXRInputStream");
    if (!clazz) {
        throw JavaExc("Could not find the JVM class id for class "
            "edu.cornell.graphics.exr.io.EXRInputStream");
    } else {
        // Keep a live reference to ensure the methodIDs remain valid
        clazz = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (!clazz) {
            throw JavaExc("Could not create global reference");
        }
    }
    read = env->GetMethodID(clazz, "read","(Ljava/nio/ByteBuffer;)Z");
    getPosition = env->GetMethodID(clazz, "position", "()J");
    setPosition = env->GetMethodID(clazz, "position", "(J)V");
    if (!read || !getPosition || !setPosition) {
        throw JavaExc("Could not get the JVM method ids for class "
            "edu.cornell.graphics.exr.io.EXRInputStream");
    }
}



void EXRJavaInputStream::initJVMData(JNIEnv* env)
{
    assert(env != NULL);
    if (!jvmData) {
        Lock lock(mutex);
        if (!jvmData) {
            jvmData = new JVMData(env);
        }
    }
}



EXRJavaInputStream::EXRJavaInputStream(JNIEnv* env, jobject stream) :
Imf::IStream("edu.cornell.graphics.exr.io.EXRInputStream"),
m_stream(NULL)
{
    assert(env != NULL);
    initJVMData(env);

    if (stream == NULL) {
        throw Iex::NullExc("EXRJavaInputStream::EXRJavaInputStream: "
            "null Java stream");
    }
    if (!env->IsInstanceOf(stream, jvmData->clazz)) {
        throw JavaExc("EXRJavaInputStream::EXRJavaInputStream: "
            "argument not an instance of "
            "edu.cornell.graphics.exr.io.EXRInputStream");
    }
    m_stream = env->NewGlobalRef(stream);
    if (m_stream == NULL) {
        throw JavaExc("Could not create global reference to the stream");
    }
}



EXRJavaInputStream::~EXRJavaInputStream()
{
    if (m_stream != NULL) {
        if (!jvmData) {
            throw Iex::LogicExc("JNI cache not initialized");
        }
        // Get a JNIEnv from the JVM
        JNIEnv* const env = getJNIEnv(jvmData->jvm);
        env->DeleteGlobalRef(m_stream);
        m_stream = NULL;
    }
}



bool EXRJavaInputStream::read (char c[/*n*/], int n)
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    jobject buffer = env->NewDirectByteBuffer(c, n);
    jboolean result = env->CallBooleanMethod(m_stream, jvmData->read, buffer);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
    env->DeleteLocalRef(buffer);
    return result != JNI_FALSE;
}
  


Imath::Int64 EXRJavaInputStream::tellg()
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    jlong value = env->CallLongMethod(m_stream, jvmData->getPosition);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
    return static_cast<Imath::Int64>(value);
}



void EXRJavaInputStream::seekg(Imath::Int64 pos)
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    env->CallVoidMethod(m_stream, jvmData->setPosition, (jlong)pos);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
}



void EXRJavaInputStream::clear()
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    env->ExceptionClear();
}




// ------------- EXRJavaOutputStream -----------------------



EXRJavaOutputStream::JVMData::JVMData(JNIEnv* envPtr)
{
    assert(envPtr != NULL);
    JNIEnvHelper env(envPtr);

    jvm = env.getJavaVM();
    clazz = env.findClassGlobalRef("edu/cornell/graphics/exr/io/EXROutputStream");
    write = env.getMethodID(clazz, "write","(Ljava/nio/ByteBuffer;)V");
    getPosition = env.getMethodID(clazz, "position", "()J");
    setPosition = env.getMethodID(clazz, "position", "(J)V");

    jclass byteBufferClazz = env.findClassLocalRef("java/nio/ByteBuffer");
    asReadOnlyBuffer = env.getMethodID(byteBufferClazz,
        "asReadOnlyBuffer", "()Ljava/nio/ByteBuffer;");
}



void EXRJavaOutputStream::initJVMData(JNIEnv* env)
{
    assert(env != NULL);
    if (!jvmData) {
        Lock lock(mutex);
        if (!jvmData) {
            jvmData.reset(new JVMData(env));
        }
    }
}


EXRJavaOutputStream::EXRJavaOutputStream(JNIEnv* env, jobject stream) :
Imf::OStream("edu.cornell.graphics.exr.io.EXROutputStream")
{
    assert(env != NULL);
    initJVMData(env);

    if (stream == NULL) {
        throw Iex::NullExc("EXRJavaOutputStream::EXRJavaOutputStream: "
            "null Java stream");
    }
    if (!env->IsInstanceOf(stream, jvmData->clazz)) {
        throw JavaExc("EXRJavaOutputStream::EXRJavaOutputStream: "
            "argument not an instance of "
            "edu.cornell.graphics.exr.io.EXROutputStream");
    }
    m_stream = env->NewGlobalRef(stream);
    if (m_stream == NULL) {
        throw JavaExc("Could not create global reference to the stream");
    }
}



EXRJavaOutputStream::~EXRJavaOutputStream()
{
    if (m_stream != NULL) {
        if (!jvmData) {
            throw Iex::LogicExc("JNI cache not initialized");
        }
        // Get a JNIEnv from the JVM
        JNIEnv* const env = getJNIEnv(jvmData->jvm);
        env->DeleteGlobalRef(m_stream);
        m_stream = NULL;
    }
}



void EXRJavaOutputStream::write(const char c[/*n*/], int n)
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    jobject buffer = env->NewDirectByteBuffer(const_cast<char*>(c), n);
    jobject readBuffer=env->CallObjectMethod(buffer,jvmData->asReadOnlyBuffer);
    env->DeleteLocalRef(buffer);
    env->CallVoidMethod(m_stream, jvmData->write, readBuffer);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
    env->DeleteLocalRef(readBuffer);
}
  


Imath::Int64 EXRJavaOutputStream::tellp()
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    jlong value = env->CallLongMethod(m_stream, jvmData->getPosition);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
    return static_cast<Imath::Int64>(value);
}



void EXRJavaOutputStream::seekp(Imath::Int64 pos)
{
    if (!jvmData) {
        throw Iex::LogicExc("JNI cache not initialized");
    }
    JNIEnv* const env = getJNIEnv(jvmData->jvm);
    env->CallVoidMethod(m_stream, jvmData->setPosition, (jlong)pos);
    if (env->ExceptionCheck() != JNI_FALSE) {
        throw JavaExc();
    }
}

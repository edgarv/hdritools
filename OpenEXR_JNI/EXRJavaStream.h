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

// Classes to create OpenEXR streams which encapsulate the abstract Java
// input/output streams

#pragma once
#if !defined(EXRJAVASTREAM_H)
#define EXRJAVASTREAM_H

#include <ImfIO.h>
#include <jni.h>

#include <memory>

class EXRJavaInputStream : public Imf::IStream
{
public:
    // stream is a non-null instance of edu.cornell.graphics.exr.io.EXRInputStream
    EXRJavaInputStream(JNIEnv* env, jobject stream);

    virtual ~EXRJavaInputStream();

    virtual bool read (char c[/*n*/], int n) override;
    
    virtual Imath::Int64 tellg() override;

    virtual void seekg(Imath::Int64 pos) override;

    virtual void clear() override;

private:

    static void initJVMData(JNIEnv* env);

    struct JVMData {
        JavaVM* jvm;
        jclass clazz;
        jmethodID read;
        jmethodID getPosition;
        jmethodID setPosition;

        JVMData(JNIEnv* env);
    };

    static volatile JVMData* jvmData;
    jobject m_stream;
};



class EXRJavaOutputStream : public Imf::OStream
{
public:
    // stream is a non-null instance of edu.cornell.graphics.exr.io.EXROutputStream
    EXRJavaOutputStream(JNIEnv* env, jobject stream);

    virtual ~EXRJavaOutputStream();

    virtual void write(const char c[/*n*/], int n) override;

    virtual Imath::Int64 tellp() override;

    virtual void seekp(Imath::Int64 pos) override;

private:

    static void initJVMData(JNIEnv* env);

    struct JVMData {
        JavaVM* jvm;
        jclass clazz;
        jmethodID write;
        jmethodID getPosition;
        jmethodID setPosition;
        jmethodID asReadOnlyBuffer;

        JVMData(JNIEnv* env);
    };

    static std::unique_ptr<volatile JVMData> jvmData;
    jobject m_stream;
};

#endif // EXRJAVASTREAM_H

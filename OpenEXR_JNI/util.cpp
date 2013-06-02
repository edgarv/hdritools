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

#include "util.h"

#include <jni.h>
#include <ImfArray.h>
#include <Iex.h>
#include <ImfRgbaFile.h>
#include <cassert>

#include <ImfStandardAttributes.h>
#include <string>


// An utility function to "throw" java exceptions. When this methods are called
// the JVM is instructed to throw an exception but the C++ control flow does
// not change. Taken from:
//    http://java.sun.com/docs/books/jni/html/exceptions.html
//
void JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg) {

    jclass cls = env->FindClass(name);
    /* if cls is NULL, an exception has already been thrown */
    if (cls != NULL) {
        env->ThrowNew(cls, msg);
    }
    /* free the local ref */
    env->DeleteLocalRef(cls);
}



JNIEnvHelper::JNIEnvHelper(JNIEnv* env) : m_env(env)
{
    assert(env != NULL);
}

JavaVM* JNIEnvHelper::getJavaVM()
{
    JavaVM* jvm;
    if (m_env->GetJavaVM(&jvm) != JNI_OK) {
        throw JavaExc("Could not get pointer to the Java VM");
    }
    return jvm;
}

jclass JNIEnvHelper::findClassGlobalRef(const char* name) {
    jclass localClass = m_env->FindClass(name);
    if (!localClass) {
        JavaExc ex("Could not find the JVM id for class ");
        ex.append(name);
        throw ex;
    }
    jclass globalClass = reinterpret_cast<jclass>(
        m_env->NewGlobalRef(localClass));
    if (!globalClass) {
        JavaExc ex("Could not create a global reference for class ");
        ex.append(name);
        throw ex;
    }
    return globalClass;
}

jclass JNIEnvHelper::findClassLocalRef(const char* name) {
    jclass localClass = m_env->FindClass(name);
    if (!localClass) {
        JavaExc ex("Could not find the JVM id for class ");
        ex.append(name);
        throw ex;
    }
    return localClass;
}

jmethodID JNIEnvHelper::getMethodID(jclass clazz,
                                    const char* name, const char* signature)
{
    jmethodID method = m_env->GetMethodID(clazz, name, signature);
    if (!method) {
        JavaExc ex("Could not get the JVM method id for ");
        ex.append(name);
        throw ex;
    }
    return method;
}

jmethodID JNIEnvHelper::getStaticMethodID(jclass clazz,
                                          const char* name, const char* sig)
{
    jmethodID method = m_env->GetStaticMethodID(clazz, name, sig);
    if (!method) {
        JavaExc ex("Could not get the JVM static method id for ");
        ex.append(name);
        throw ex;
    }
    return method;
}

jfieldID JNIEnvHelper::getFieldID(jclass clazz,
                                  const char* name, const char* signature)
{
    jfieldID field = m_env->GetFieldID(clazz, name, signature);
    if (!field) {
        JavaExc ex("Could not get the JVM field id for ");
        ex.append(name);
        throw ex;
    }
    return field;
}

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

#include <jni.h>
#include "util.h"

#include <ImfThreading.h>



// DLL-main for JNI
jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    union {
        JNIEnv* envPtr;
        void* void_env;
    };
    if (vm->GetEnv(&void_env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    try {
        JNIEnvHelper env(envPtr);
        jclass clazz = env.findClassLocalRef("java/lang/Runtime");
        jmethodID getRuntimeID = env.getStaticMethodID(clazz,
            "getRuntime", "()Ljava/lang/Runtime;");
        jmethodID availableProcessorsID = env.getMethodID(clazz,
            "availableProcessors", "()I");
        jobject runtime = envPtr->CallStaticObjectMethod(clazz, getRuntimeID);
        if (runtime != NULL) {
            int count = envPtr->CallIntMethod(runtime, availableProcessorsID);
            Imf::setGlobalThreadCount(count);
        }
    } catch (...) {
        // nothing to do
    }

    return JNI_VERSION_1_6;
}


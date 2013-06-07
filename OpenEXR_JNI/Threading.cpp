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

#include <edu_cornell_graphics_exr_Threading.h>
#include "util.h"

#include <ImfThreading.h>


jint Java_edu_cornell_graphics_exr_Threading_globalThreadCount(
    JNIEnv* env, jclass)
{
    int count = safeCall(env, [](JNIEnv* env) {
        return Imf::globalThreadCount();
    }, static_cast<int>(0));
    return count;
}

void Java_edu_cornell_graphics_exr_Threading_setGlobalThreadCount(
    JNIEnv* env, jclass, jint count)
{
    safeCall(env, [count](JNIEnv* env) {
        Imf::setGlobalThreadCount(count);
    });
}

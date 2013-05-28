/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#include <half.h>
#include <Iex.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>

#include <IlmThread.h>
#include <IlmThreadPool.h>

#include <fstream>
#include <cassert>

#include <edu_cornell_graphics_exr_EXRSimpleImage.h>

#include "util.h"
#if !USE_JAVA_UTF8
  #include "UnicodeStream.h"
#endif


// Our favourite exception for this DLL
const char *EXCEPTION = "fileformat/OpenEXRFormat$OpenEXRIOException";
const char *EXR_EXCEPTION = "edu/cornell/graphics/exr/EXRIOException";

// DLL-main for JNI
jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    union {
        JNIEnv* env;
        void* void_env;
    };
    if (vm->GetEnv(&void_env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}

// Such a simple method, but will save painful debugging
jlong JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_getNativeVersion
  (JNIEnv *, jclass)
{
#if defined(__GNUC__) || defined(__clang__)
	return __extension__ edu_cornell_graphics_exr_EXRSimpleImage_serialVersionUID;
#else
	return edu_cornell_graphics_exr_EXRSimpleImage_serialVersionUID;
#endif
}

// Sets the number of global working threads, or throws a nasty exception
void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_setNumWorkingThreads
  (JNIEnv *env, jclass, jint numThreads)
{
	assert(numThreads >= 0);
	if (IlmThread::supportsThreads()) {

		IlmThread::ThreadPool::globalThreadPool().setNumThreads(numThreads);
	}
	else {

		JNU_ThrowByName(env,
			"java/lang/UnsupportedOperationException",
			"The IlmThread library doesn't support threads.");
	}
}

// Initializes the type caches
JNIEXPORT void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_initCache
  (JNIEnv *env, jclass)
{
	try {
		Attributes::initCache(env);
		OpenEXRTo::initCache(env);
	}
	catch(std::exception & e) {
		if (! env->ExceptionCheck()) {
			JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
		}
	}
}


/*
 * Class:     edu_cornell_graphics_exr_EXRSimpleImage
 * Method:    read
 * Signature: (Ledu/cornell/graphics/exr/EXRSimpleImage/OpenEXRTo;ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_read
  (JNIEnv *env, jclass cls, jobject jexrTo, jint numChannels, jstring jfilename)
{

#if USE_JAVA_UTF8
	const char * filename = env->GetStringUTFChars(jfilename, NULL);
#else
	const jchar * filename = env->GetStringChars(jfilename, NULL);
#endif

	try {
		if (filename == NULL) {
			throw JavaExc("Null filename.");
		}

#if USE_JAVA_UTF8
		Imf::RgbaInputFile file(filename);
#else
		const jint filenameLen = env->GetStringLength(jfilename);
		UnicodeIFStream imfIs(filename, filenameLen);
		Imf::RgbaInputFile file(imfIs);
#endif
		Imath::Box2i dw = file.dataWindow();
		const int width  = dw.max.x - dw.min.x + 1;
		const int height = dw.max.y - dw.min.y + 1;

		// Memory for reading the all pixels
		Imf::Array2D<Imf::Rgba> halfPixels(width, height);
		
		// Read all the pixels from the image
		file.setFrameBuffer (&halfPixels[0][0], 1, width);
		file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);

		// And saves that into the transfer object
		saveToTransferObject(env, jexrTo, file.header(), 
			halfPixels, width, height, numChannels);
	}
	catch (std::exception &e) {
		if (! env->ExceptionCheck() ) {
			// Something ugly has happened, so we throw that exception to Java
			// ... unless something so bad occured that an exception is already raised!
			JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
		}
	}

#if USE_JAVA_UTF8
	env->ReleaseStringUTFChars(jfilename, filename);
#else
	// Release the filename
	env->ReleaseStringChars(jfilename, filename);
#endif

}


/*
 * Class:     edu_cornell_graphics_exr_EXRSimpleImage
 * Method:    write
 * Signature: (Ljava/lang/String;[FIIILedu/cornell/graphics/exr/Attributes;I)V
 */
JNIEXPORT void JNICALL Java_edu_cornell_graphics_exr_EXRSimpleImage_write
  (JNIEnv *env, jclass, jstring jfilename, jfloatArray jpixelBuffer, 
   jint width, jint height, jint numChannels, jobject jattrib, jint compressionFlag)
{
	assert(numChannels == 3 || numChannels == 4);
	
#if USE_JAVA_UTF8
	const char * filename = env->GetStringUTFChars(jfilename, NULL);
#else
	// Get the Unicode filename
	const jchar * filename = env->GetStringChars(jfilename, NULL);
#endif

	try {
		if (filename == NULL) {
			throw JavaExc("Null filename.");
		}
		
		// Converted version of the memory
		Imf::Array2D<Imf::Rgba> halfPixels(width, height);

		// Convert the pixels according to the channel configuration
		jboolean isBufferCopy;
		const float *pixels = (const float*)env->GetPrimitiveArrayCritical(jpixelBuffer, &isBufferCopy);

		const int numPixels = width*height;
		Imf::Rgba * halfPixelsPtr = &halfPixels[0][0];
		if (numChannels == 3) {
			for(int i = 0; i < numPixels; ++i) {
				const int base = i*3;
				halfPixelsPtr[i].r = pixels[base];
				halfPixelsPtr[i].g = pixels[base+1];
				halfPixelsPtr[i].b = pixels[base+2];
			}
		}
		else if (numChannels == 4) {
			for(int i = 0; i < numPixels; ++i) {
				const int base = i*4;
				halfPixelsPtr[i].r = pixels[base];
				halfPixelsPtr[i].g = pixels[base+1];
				halfPixelsPtr[i].b = pixels[base+2];
				halfPixelsPtr[i].a = pixels[base+3];
			}
		}
		
		const Imf::RgbaChannels channels = numChannels == 3 ? Imf::WRITE_RGB : Imf::WRITE_RGBA;
		env->ReleasePrimitiveArrayCritical(jpixelBuffer, const_cast<float*>(pixels), 0);

		Imf::Header hd(width, height,
			   /* pixelAspectRatio */   1,
			   /* screenWindowCenter */ Imath::V2f(0,0),
			   /* screenWindowWidth */  1,
			   /* lineOrder */          Imf::INCREASING_Y,
			   /* compression) */       static_cast<Imf::Compression>(compressionFlag) );

		if(jattrib != NULL) {
			Attributes attrib(jattrib);
			attrib.setHeaderAttribs(env, hd);
		}

		// Actually write the file
#if USE_JAVA_UTF8
		Imf::RgbaOutputFile file(filename, hd, channels);
#else
		const jint filenameLen = env->GetStringLength(jfilename);
		UnicodeOFStream omfOs(filename, filenameLen);
		Imf::RgbaOutputFile file(omfOs, hd, channels);
#endif
		file.setFrameBuffer(&halfPixels[0][0], 1, width);
		file.writePixels(height);

	}
	catch (std::exception &e) {
		// Something ugly has happened, so we throw that exception to Java
		JNU_ThrowByName(env, EXR_EXCEPTION, e.what());
	}


	// Release the filename
#if USE_JAVA_UTF8
	env->ReleaseStringUTFChars(jfilename, filename);
#else
	env->ReleaseStringChars(jfilename, filename);
#endif

}

/*
 * Utility functions header for the RTGI oriented OpenEXR JNI binding.
 *
 * Edgar Velazquez-Armendariz - eva5 [at] cs_cornell_edu
 * August 2007.
 */

#if !defined( UTIL_H )
#define UTIL_H

#include <jni.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>

// An utility function to "throw" java exceptions. When these methods are called
// the JVM is instructed to throw an exception but the C++ control flow does
// not change. Taken from:
//    http://java.sun.com/docs/books/jni/html/exceptions.html
//
void JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg);

// Utility function to convert in place a buffer of width*height float RGB pixels
// into half RGBA pixels.
void convertToHalfRGBA(float *rgbPixels, const int width, const int height, 
					   const half defaultAlpha = 1.0f);


// Utility function to allocate a new array which will hold the float RGB version of
// the given image. It assumes that everything is properly allocated and setup!
// It also assumes that sizeof(float) = 4
float* convertToFloatRGB(const Imf::Array2D<Imf::Rgba> &halfImage, 
						 const int width, const int height);


/*
 * Saves the given buffer of float RGB pixels and its dimensions into the Java transfer object
 */
void saveToTransferObject(JNIEnv *env, jobject jTo, 
						  const float *rgbPixels, const int width, const int height);



#endif // UTIL_H

/*
 * Utility functions for the RTGI oriented OpenEXR JNI binding.
 *
 * Edgar Velazquez-Armendariz - eva5 [at] cs_cornell_edu
 * August 2007.
 */

#include "util.h"

#include <jni.h>
#include <ImfArray.h>
#include <Iex.h>
#include <ImfRgbaFile.h>
#include <cassert>


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


// Utility function to convert in place a buffer of width*height float RGB pixels
// into half RGBA pixels.
void convertToHalfRGBA(float *rgbPixels, const int width, const int height, 
					   const half defaultAlpha) {

	// By contract the memory pointed by rgbPixels is not needed, so I will use
	// it as the destination. Note that even when using full RGBA pixels, the size
	// will be enough: rgbPixels size = (3 * 4) >  required size = (4 * 2)
	Imf::Rgba *halfPixels = reinterpret_cast<Imf::Rgba*>(rgbPixels);
	
	// Convert all pixels in order
	for (int i = 0; i < width*height; ++i) {

		// There is no overwriting, thus we make the conversion completely in place.
		halfPixels->r = *rgbPixels++;
		halfPixels->g = *rgbPixels++;
		halfPixels->b = *rgbPixels++;
		halfPixels->a = defaultAlpha;

		++halfPixels;
	}
}


// Utility function to allocate a new array which will hold the float RGB version of
// the given image. It assumes that everything is properly allocated and setup!
// It also assumes that sizeof(float) = 4
float* convertToFloatRGB(const Imf::Array2D<Imf::Rgba> &halfImage, 
						 const int width, const int height) {

	// Our base pointer
	const Imf::Rgba *halfPixels = &halfImage[0][0];

	// Allocates the new data
	float *rgbPixels = new float[width * height * 4];
	float *base = rgbPixels;

	// Converts all pixels
	for (int i = 0; i < width*height; ++i, ++halfPixels) {

		*rgbPixels++ = halfPixels->r;
		*rgbPixels++ = halfPixels->g;
		*rgbPixels++ = halfPixels->b;
	}

	return base;
}



/*
 * Saves the given buffer of float RGB pixels and its dimensions into the Java transfer object
 */
void saveToTransferObject(JNIEnv *env, jobject jTo, 
						  const float *rgbPixels, const int width, const int height) {

	// Create the Java ByteBuffer to encapsule the rgbPixels
	jobject jDataBuffer = 
		env->NewDirectByteBuffer(const_cast<float*>(rgbPixels), width*height*3*4);
	if (jDataBuffer == NULL) {
		Iex::throwErrnoExc("JNI could not create the Direct Buffer.");
	}

	// Class of the transferObjet
	jclass toClass = env->GetObjectClass(jTo);
	assert(toClass != NULL);
	
	// IDs for the fields
	jfieldID dataBufferID = env->GetFieldID(toClass, "dataBuffer", "Ljava/nio/ByteBuffer;");
	jfieldID widthID      = env->GetFieldID(toClass, "width", "I");
	jfieldID heightID     = env->GetFieldID(toClass, "height", "I");

	// Consistency checks
	assert(dataBufferID != NULL);
	assert(widthID != NULL);
	assert(heightID != NULL);

	// Set the dataBuffer, width and height fields of the TO instance
	env->SetObjectField(jTo, dataBufferID, jDataBuffer);
	env->SetIntField(jTo, widthID, width);
	env->SetIntField(jTo, heightID, height);

}

/*
 * Utility functions header for the RTGI oriented OpenEXR JNI binding.
 *
 * Edgar Velazquez-Armendariz - eva5 [at] cs_cornell_edu
 * August 2007 - October 2008.
 */

#if !defined( UTIL_H )
#define UTIL_H

#include <jni.h>
#include <string>
#include <Iex.h>


// Forward declarations
namespace Imf {
	class Header;
	struct Rgba;

	template<class T>
	class Array2D;
}

// Construct a Java Exception extending IExBase
DEFINE_EXC (JavaExc, Iex::BaseExc) 


// An utility function to "throw" java exceptions. When these methods are called
// the JVM is instructed to throw an exception but the C++ control flow does
// not change. Taken from:
//    http://java.sun.com/docs/books/jni/html/exceptions.html
//
void JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg);

/**
 * Saves the half buffer into the transfer object, converting the data to 32-bit floating point.
 * It receives either 3 or 4 as the number or channels parameter (those are the channels
 * written into the float buffer: full RGBA or just RGB).
 */
void saveToTransferObject(JNIEnv *env, jobject jTo,
						  const Imf::Header &header,
						  const Imf::Array2D<Imf::Rgba> &halfPixels,
						  const int width, const int height,
						  const int numChannels);

/** 
 * Class to handle stuff related to the edu.cornell.graphics.exr.Attributes class.
 * Instances of this class should not survive between JNI calls (JVM->C++)
 */
class Attributes {

private:
	static jfieldID ownerID;
	static jfieldID commentsID;
	static jfieldID capDateID;
	static jfieldID utcOffsetID;
	static jmethodID constructorID;
	static bool isCacheUpdated;

	// An actual java instance of this class
	jobject instance;

	// Sets one of the strings fields
	void setStringField(JNIEnv *env, jfieldID fid, const char * val);
	void setStringField(JNIEnv *env, jfieldID fid, const std::string & val);

	// Sets float fields
	void setFloatField(JNIEnv *env, jfieldID fid, jfloat val);

	// Sets the value of a string field into the header using the
	// function pointed by attribMethod if it's not null
	void setStringAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
		void (*attribMethod)(Imf::Header &, const std::string &) );

	// Sets the value of a float field into the header using the
	// function pointed by attribMethod, it it's not NaN or Infinity.
	void setFloatAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
		void (*attribMethod)(Imf::Header &, const float &) );

public:

	// This needs to be called before anything!
	static void initCache(JNIEnv *env);

	Attributes(JNIEnv *env, const Imf::Header & header);


	// The constructor assumes that the given object is a non-null
	// instance of Attributes!
	Attributes(jobject attrib);

	// Returns the java instance version of the attributes object
	jobject getInstance() const {
		return instance;
	}

	// Adds the attributes to the header
	void setHeaderAttribs(JNIEnv *env, Imf::Header & header);

};


/** 
 * Lightweight class to handle stuff related to 
 * edu.cornell.graphics.exr.EXRSimpleImage$OpenEXRTo 
 */
class OpenEXRTo {

private:
	static jfieldID attribID;
	static jfieldID widthID;
	static jfieldID heightID;
	static jfieldID bufferID;
	static bool isCacheUpdated;

	// An actual java instance of this class
	jobject instance;

public:

	// Call before using any other method!
	static void initCache(JNIEnv *env);

	/** Basic constructor: it is just a wrapper for a non null instance */
	OpenEXRTo(jobject obj);

	// Setters which modify the underlying java instance
	void setWidth(JNIEnv *env, int width);
	void setHeight(JNIEnv *env, int height);
	void setAttributes(JNIEnv *env, const Attributes & attrib);
	void setBuffer(JNIEnv *env, jobject buffer);
};


#endif // UTIL_H

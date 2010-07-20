/*
 * Utility functions for the RTGI oriented OpenEXR JNI binding.
 *
 * Edgar Velazquez-Armendariz - eva5 [at] cs_cornell_edu
 * August 2007 - October 2008.
 */

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


// Initialize Attributes' static data
jfieldID Attributes::ownerID = NULL;
jfieldID Attributes::commentsID = NULL;
jfieldID Attributes::capDateID = NULL;
jfieldID Attributes::utcOffsetID = NULL;
jmethodID Attributes::constructorID = NULL;
bool Attributes::isCacheUpdated = false;


void Attributes::setStringField(JNIEnv *env, jfieldID fid, const char * val){
	assert(instance != NULL);
	jstring jstr = env->NewStringUTF(val);
	if (jstr == NULL) {
		throw JavaExc("JVM constructor returned null");
	}
	env->SetObjectField(instance, fid, jstr);
	if (env->ExceptionCheck()) {
		env->DeleteLocalRef(jstr);
		throw JavaExc("Java exception occurred.");
	}
	env->DeleteLocalRef(jstr);
}

void Attributes::setStringField(JNIEnv *env, jfieldID fid, const std::string & val) {
	setStringField(env, fid, val.c_str());
}

void Attributes::setFloatField(JNIEnv *env, jfieldID fid, jfloat val) {
	assert(instance != NULL);
	env->SetFloatField(instance, fid, val);
	if (env->ExceptionCheck()) {
		throw JavaExc("Java exception occurred.");
	}
}

void Attributes::setStringAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
	void (*attribMethod)(Imf::Header &, const std::string &) )
{
	assert(attribMethod != NULL);
	jstring jstr = (jstring)env->GetObjectField(instance, fid);
	if (jstr != NULL) {
		const char * str = env->GetStringUTFChars(jstr, NULL);
		if (str == NULL) {
			env->ReleaseStringUTFChars(jstr, str);
			throw JavaExc("Unexpected NULL String.");
		}
		(*attribMethod)(header, str);
		env->ReleaseStringUTFChars(jstr, str);
	}
}

void Attributes::setFloatAttrib(JNIEnv *env, Imf::Header & header, jfieldID fid,
	void (*attribMethod)(Imf::Header &, const float &) ) 
{
	assert(attribMethod != NULL);
	union {jfloat val; unsigned int bits;};
	val = env->GetFloatField(instance, fid);
	// Add if the value is not NaN nor INFINITY
	if (bits != 0x7fc00000 && bits != 0x7f800000 && bits != 0xff800000) {
		(*attribMethod)(header, val);
	}
}

void Attributes::initCache(JNIEnv *env) 
{
	jclass attribClass = env->FindClass("edu/cornell/graphics/exr/Attributes");
	if (attribClass == NULL) { throw JavaExc("attribClass NULL"); }

	ownerID = env->GetFieldID(attribClass, "owner", "Ljava/lang/String;");
	if (ownerID == NULL) { 
		env->DeleteLocalRef(attribClass);
		throw JavaExc("ownerID NULL"); 
	}

	commentsID = env->GetFieldID(attribClass, "comments", "Ljava/lang/String;");
	if (commentsID == NULL) { 
		env->DeleteLocalRef(attribClass);
		throw JavaExc("commentsID NULL"); 
	}

	capDateID = env->GetFieldID(attribClass, "capDate", "Ljava/lang/String;");
	if (capDateID == NULL) { 
		env->DeleteLocalRef(attribClass);
		throw JavaExc("capDateID NULL"); 
	}

	utcOffsetID = env->GetFieldID(attribClass, "utcOffset", "F");
	if (utcOffsetID == NULL) { 
		env->DeleteLocalRef(attribClass);
		throw JavaExc("utcOffsetID NULL"); 
	}

	// We'll use the explicit boolean constructor
	constructorID = env->GetMethodID(attribClass, "<init>", "(Z)V");
	if (constructorID == NULL) { 
		env->DeleteLocalRef(attribClass);
		throw JavaExc("constructorID NULL"); 
	}

	env->DeleteLocalRef(attribClass);
	isCacheUpdated = true;
}

Attributes::Attributes(JNIEnv *env, const Imf::Header & header) : instance(NULL) 
{
	assert(isCacheUpdated);

	// Gets which attributes are in the header. If none is set
	// don't create the attributes object.
	const bool hasComments  = Imf::hasComments(header);
	const bool hasOwner     = Imf::hasOwner(header);
	const bool hasCapDate   = Imf::hasCapDate(header);
	const bool hasUtcOffset = Imf::hasUtcOffset(header);

	// Note that it's not possible to cache the class, if we do we would
	// get an instance of InvocationTargetException or other bizarre things instead!!
	jclass attribClass = env->FindClass("edu/cornell/graphics/exr/Attributes");
	if (attribClass == NULL) { throw JavaExc("attribClass NULL"); }

	// Don't initialize anything: leave all the defaults
	instance = env->NewObject(attribClass, constructorID, JNI_FALSE);
	if (env->ExceptionCheck()) {
		env->DeleteLocalRef(attribClass);
		throw JavaExc("Java exception occurred.");
	}

	// Sets whatever fields are there
	try {
		if (hasComments) {
			setStringField(env, commentsID, Imf::comments(header) );
		}
		if (hasOwner) {
			setStringField(env, ownerID, Imf::owner(header) );
		}
		if (hasCapDate) {
			setStringField(env, capDateID, Imf::capDate(header) );
		}
		if (hasUtcOffset) {
			setFloatField(env, utcOffsetID, Imf::utcOffset(header) );
		}
		else if (hasCapDate) {
			setFloatField(env, utcOffsetID, 0.0f );
		}
	}
	catch(std::exception &e) {
		env->DeleteLocalRef(attribClass);
		throw e;
	}

	env->DeleteLocalRef(attribClass);
}


Attributes::Attributes(jobject attrib) : instance(attrib) {
	assert(isCacheUpdated);
}


// Adds the attributes to the header
void Attributes::setHeaderAttribs(JNIEnv *env, Imf::Header & header)
{
	assert(isCacheUpdated);
	if (instance == NULL) { return; }

	// Sets the attributes in the header for the valid fields
	setStringAttrib(env, header, ownerID,     &Imf::addOwner);
	setStringAttrib(env, header, commentsID,  &Imf::addComments);
	setStringAttrib(env, header, capDateID,   &Imf::addCapDate);
	setFloatAttrib( env, header, utcOffsetID, &Imf::addUtcOffset);
}





// Initial values for OpenEXRTo static data
jfieldID OpenEXRTo::attribID = NULL;
jfieldID OpenEXRTo::widthID = NULL;
jfieldID OpenEXRTo::heightID = NULL;
jfieldID OpenEXRTo::bufferID = NULL;
bool OpenEXRTo::isCacheUpdated = false;

OpenEXRTo::OpenEXRTo(jobject obj) : instance(obj) {
	assert(instance != NULL);
}

void OpenEXRTo::setWidth(JNIEnv *env, int width) {
	assert(isCacheUpdated);
	env->SetIntField(instance, widthID, width);
	if (env->ExceptionCheck()) {
		throw JavaExc("Java exception occurred.");
	}
}

void OpenEXRTo::setHeight(JNIEnv *env, int height) {
	assert(isCacheUpdated);
	env->SetIntField(instance, heightID, height);
	if (env->ExceptionCheck()) {
		throw JavaExc("Java exception occurred.");
	}
}

void OpenEXRTo::setAttributes(JNIEnv *env, const Attributes & attrib) {
	assert(isCacheUpdated);
	env->SetObjectField(instance, attribID, attrib.getInstance());
	if (env->ExceptionCheck()) {
		throw JavaExc("Java exception occurred.");
	}
}

void OpenEXRTo::setBuffer(JNIEnv *env, jobject buffer) {
	assert(isCacheUpdated);
	env->SetObjectField(instance, bufferID, buffer);
	if (env->ExceptionCheck()) {
		throw JavaExc("Java exception occurred.");
	}
}

void OpenEXRTo::initCache(JNIEnv *env)
{
	jclass toClass = env->FindClass("edu/cornell/graphics/exr/EXRSimpleImage$OpenEXRTo");
	if (toClass == NULL) { throw JavaExc("toClass NULL"); }

	attribID = env->GetFieldID(toClass, "attrib", "Ledu/cornell/graphics/exr/Attributes;");
	if (attribID == NULL) { 
		env->DeleteLocalRef(toClass);
		throw JavaExc("attribID NULL"); 
	}

	widthID = env->GetFieldID(toClass, "width", "I");
	if (widthID == NULL) { 
		env->DeleteLocalRef(toClass);
		throw JavaExc("widthID NULL"); 
	}

	heightID = env->GetFieldID(toClass, "height", "I");
	if (heightID == NULL) { 
		env->DeleteLocalRef(toClass);
		throw JavaExc("heightID NULL"); 
	}

	bufferID = env->GetFieldID(toClass, "buffer", "[F");
	if (bufferID == NULL) { 
		env->DeleteLocalRef(toClass);
		throw JavaExc("bufferID NULL"); 
	}

	env->DeleteLocalRef(toClass);
	isCacheUpdated = true;
}


// Updated save function with support for attributes
void saveToTransferObject(JNIEnv *env, jobject jTo,
	  const Imf::Header &header,
	  const Imf::Array2D<Imf::Rgba> &halfPixels,
	  const int width, const int height, const int numChannels)
{
	assert(numChannels == 3 || numChannels == 4);
	OpenEXRTo to(jTo);

	// Start with the basics: width & height
	to.setWidth(env, width);
	to.setHeight(env, height);

	// Conversion from half to float
	const jsize numElements = width*height*numChannels;
	jfloatArray jbuffer = env->NewFloatArray(numElements);
	if (jbuffer == NULL) { return; /* exception thrown */ }

	// This method has greater chances to get the actual array data without extra copies
	jboolean isBufferCopy;
	float * pixels = (float*)env->GetPrimitiveArrayCritical(jbuffer, &isBufferCopy);

	const Imf::Rgba * halfPixelsPtr = &halfPixels[0][0];

	if (numChannels == 3) {

		for(int i = 0; i < width*height; ++i) {
			const int base = i*3;
			const Imf::Rgba & px = halfPixelsPtr[i];
			pixels[base]   = px.r;
			pixels[base+1] = px.g;
			pixels[base+2] = px.b;
		}

	}
	else if (numChannels == 4) {

		for(int i = 0; i < width*height; ++i) {
			const int base = i*4;
			const Imf::Rgba & px = halfPixelsPtr[i];
			pixels[base]   = px.r;
			pixels[base+1] = px.g;
			pixels[base+2] = px.b;
			pixels[base+3] = px.a;
		}
	}
	env->ReleasePrimitiveArrayCritical(jbuffer, pixels, 0);
	to.setBuffer(env, jbuffer);

	// Gets which attributes are in the header. If none is set
	// don't create/set the attributes object.
	if (Imf::hasComments(header) || Imf::hasOwner(header) || 
		Imf::hasCapDate(header)  || Imf::hasUtcOffset(header))
	{
		Attributes attrib(env, header);
		to.setAttributes(env, attrib);
	}
}

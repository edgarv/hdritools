#include <half.h>
#include <Iex.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <cassert>

#include "jni/fileformat_OpenEXRFormat.h"
#include "util.h"


// Our favourite exception for this DLL
const char *EXCEPTION = "fileformat/OpenEXRFormat$OpenEXRIOException";


/*
 * Class:     fileformat_OpenEXRFormat
 * Method:    toHalf
 * Signature: (F)S
 */
JNIEXPORT jshort JNICALL Java_fileformat_OpenEXRFormat_toHalf
(JNIEnv *, jclass, jfloat n) {

	jshort result;
	unsigned short uhalf = half(n).bits();

	// I only want to copy the bits, so I perform a little trick here
	jshort *aux = reinterpret_cast<jshort*>(&uhalf);
	result = *aux;
	return result;
}

/*
 * Class:     fileformat_OpenEXRFormat
 * Method:    write
 * Signature: (Ljava/lang/String;Ljava/nio/ByteBuffer;IIIZ)V
 */
JNIEXPORT void JNICALL Java_fileformat_OpenEXRFormat_write
  (JNIEnv *env, jclass clazz, jstring jfilename, jobject jpixels, jint width, jint height, 
   jint compressionFlag,jboolean isBufferHalfRGBA)
{
	
	// We retrieve the name of the file
	const char *filename = env->GetStringUTFChars(jfilename, 0);

	// This was the point to keep a directbuffer from Java
	float *rgbPixels = reinterpret_cast<float*>(env->GetDirectBufferAddress(jpixels));
	
	// Throws a Java Exception and returns if we don't have a buffer
	if (rgbPixels == NULL) {
		JNU_ThrowByName(env, EXCEPTION, "JNI could not retrieve the Direct Buffer address.");
		return;
	}

	// Paranoid validation
	assert(fileformat_OpenEXRFormat_NO_COMPRESSION    == Imf::NO_COMPRESSION);
	assert(fileformat_OpenEXRFormat_RLE_COMPRESSION   == Imf::RLE_COMPRESSION);
	assert(fileformat_OpenEXRFormat_ZIPS_COMPRESSION  == Imf::ZIPS_COMPRESSION);
	assert(fileformat_OpenEXRFormat_ZIP_COMPRESSION   == Imf::ZIP_COMPRESSION);
	assert(fileformat_OpenEXRFormat_PIZ_COMPRESSION   == Imf::PIZ_COMPRESSION);
	assert(fileformat_OpenEXRFormat_PXR24_COMPRESSION == Imf::PXR24_COMPRESSION);
	assert(fileformat_OpenEXRFormat_B44_COMPRESSION   == Imf::B44_COMPRESSION);
	assert(fileformat_OpenEXRFormat_B44A_COMPRESSION  == Imf::B44A_COMPRESSION);

	try {

		// Performs the inplace conversion if required
		if (!isBufferHalfRGBA) {
			convertToHalfRGBA(rgbPixels, width, height);
		}
		// The base is always the same
		Imf::Rgba *halfPixelsBase = reinterpret_cast<Imf::Rgba*>(rgbPixels);

		// Hyper-easy IlmImf-based file creation:
		// Filename, width, height, channels, pixel aspect ratio, screen windows center, 
		// screen window width, line order, compression
		Imf::RgbaOutputFile file(filename, width, height, Imf::WRITE_RGB, 1, 
			Imath::V2f(0,0), 1, Imf::INCREASING_Y, static_cast<Imf::Compression>(compressionFlag));
		file.setFrameBuffer(halfPixelsBase, 1, width);
		file.writePixels(height);
	}
	catch (Iex::BaseExc &e) {
		// Something ugly has happened, so we throw that exception to Java
		JNU_ThrowByName(env, EXCEPTION, e.what());
	}



	// Cleanup time
	env->ReleaseStringUTFChars(jfilename, filename);
}


/*
 * Class:     fileformat_OpenEXRFormat
 * Method:    read
 * Signature: (Lfileformat/OpenEXRFormat$OpenEXRTo;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_fileformat_OpenEXRFormat_read
(JNIEnv *env, jclass clazz, jobject jTo, jstring jfilename) 
{

	// We retrieve the name of the file
	const char *filename = env->GetStringUTFChars(jfilename, 0);

	try {
		Imf::RgbaInputFile file(filename);

		Imath::Box2i dw = file.dataWindow();
		const int width  = dw.max.x - dw.min.x + 1;
		const int height = dw.max.y - dw.min.y + 1;

		// Memory for reading the all pixels
		Imf::Array2D<Imf::Rgba> halfPixels(width, height);
		
		// Read all the pixels from the image
		file.setFrameBuffer (&halfPixels[0][0], 1, width);
		file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);

		// Now we allocate another array which will be passed to
		// the JVM with the same array but with float RGB pixels
		float *rgbPixels = convertToFloatRGB(halfPixels, width, height);

		// And saves that into the transfer object
		saveToTransferObject(env, jTo, rgbPixels, width, height);
	}
	catch (Iex::BaseExc &e) {
		// Something ugly has happened, so we throw that exception to Java
		JNU_ThrowByName(env, EXCEPTION, e.what());
	}

	// Cleanup time
	env->ReleaseStringUTFChars(jfilename, filename);

}

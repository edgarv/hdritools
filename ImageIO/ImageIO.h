#ifndef IMAGEIO_H
#define IMAGEIO_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IMAGEIO_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IMAGEIO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef IMAGEIO_EXPORTS
#define IMAGEIO_API __declspec(dllexport)
#else
#define IMAGEIO_API __declspec(dllimport)
#endif


// This class is exported from the ImageIO.dll
class IMAGEIO_API CImageIO {
public:
	CImageIO(void);
	// TODO: add your methods here.
};

extern IMAGEIO_API int nImageIO;

IMAGEIO_API int fnImageIO(void);


#endif

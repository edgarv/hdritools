// ImageIO.cpp : Defines the exported functions for the DLL application.
//

#include "StdAfx.h"
#include "ImageIO.h"


// This is an example of an exported variable
IMAGEIO_API int nImageIO=0;

// This is an example of an exported function.
IMAGEIO_API int fnImageIO(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see ImageIO.h for the class definition
CImageIO::CImageIO()
{
	return;
}

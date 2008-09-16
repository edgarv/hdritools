/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

#include "StdAfx.h"

#include "rgbe.h"
#include <math.h>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>

#include <iomanip>

/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and 
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)

 2008.02.10 edgar   Adapted to C++ and decoupled file reading
            and pixel conversion operations.
 2002.10.29	westin	Allow additional header after FORMAT= line; such
			files are routinely emitted by RADIANCE tools.

 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/


using namespace pcg;

const unsigned int Rgbe::rgbeLUT_UI[2][256] =
	#include "rgbeLUT.h"

const float * Rgbe::rgbeCastLUT = reinterpret_cast<const float*>(&Rgbe::rgbeLUT_UI[0][0]);
const float * Rgbe::rgbeExpLUT  = reinterpret_cast<const float*>(&Rgbe::rgbeLUT_UI[1][0]);

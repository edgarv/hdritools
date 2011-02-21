//////////////////////////////////////////////////////////////////////////
//
// exrread.cpp
//
// Matlab interface for reading a float image to exr file
//
// img = exrread(filename)
//
// img is a 3d (color) hdr image (when filename is gray image, img's 3 
// channels are the same).
//
// see also exrwrite.cpp
// 
// Jinwei Gu. 2006/02/10
// jwgu@cs.columbia.edu
//
// When using mex to compiling it in matlab, make sure to use VC7.1 or above
// instead of VC6. 
//////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1600
# include <yvals.h>
// This macro is defined in ISO/IEC TR 19769:2004
# if !defined(__STDC_UTF_16__)
#  define __STDC_UTF_16__
# endif
#endif

#include <mex.h>

#include <ImathBox.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>

using namespace Imf;
using namespace Imath;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{ 
    /* Check for proper number of arguments */
    if (nrhs != 1) {
        mexErrMsgTxt("The filename is required.");
    } else if (nlhs > 1) {
        mexErrMsgTxt("Too many output arguments.");
    }
    
    char inputfile[256];
    mxGetString(prhs[0], inputfile, 256);
    
    try {
        RgbaInputFile file(inputfile);
        Box2i dw = file.dataWindow();

        const int width  = dw.max.x - dw.min.x + 1;
        const int height = dw.max.y - dw.min.y + 1;

        // Create the helper data on the Matlab heap, so that it's
        // deallocated automatically
        Rgba *pixels = (Rgba*)mxCalloc(width*height, sizeof(Rgba));

        file.setFrameBuffer(pixels-dw.min.x-dw.min.y*width, 1, width);
        file.readPixels(dw.min.y, dw.max.y);

        mwSize dims[3];
        dims[0]=height; dims[1]=width; dims[2]=3;
        plhs[0] = mxCreateNumericArray(3, dims, mxSINGLE_CLASS, mxREAL); 
        float *img = (float*)mxGetPr(plhs[0]);
        const int A = width*height;

        for(int i=0; i<height; i++)
        for(int j=0; j<width; j++)
        {
            const int k = i*width+j;
            const int m = j*height+i;

            img[m    ] = pixels[k].r;
            img[A+m  ] = pixels[k].g;
            img[2*A+m] = pixels[k].b;
        }
        
    }
    catch( std::exception &e ) {
        mexErrMsgIdAndTxt("OpenEXR:exception", e.what());
    }
    
} 

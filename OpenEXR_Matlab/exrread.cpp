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
// Modified by Edgar Velazquez-Armendariz
// <cs#cornell#edu - eva5>
//
// When using mex to compiling it in matlab, make sure to use VC7.1 or above
// instead of VC6. 
//////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1600
# define CHAR16_T wchar_t
#endif

#include <mex.h>

#include <ImathBox.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <string>

using namespace Imf;
using namespace Imath;



void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{ 
    /* Check for proper number of arguments */
    if (nrhs != 1) {
        mexErrMsgIdAndTxt("OpenEXR:argument", "The filename is required.");
    } else if (nlhs > 1) {
        mexErrMsgIdAndTxt("OpenEXR:argument", "Too many output arguments.");
    }

    char *inputfilePtr = mxArrayToString(prhs[0]);
    if (inputfilePtr == NULL) {
        mexErrMsgIdAndTxt("OpenEXR:argument", "Invalid filename argument.");
    }
    // Copy to a string so that the matlab memory may be freed asap
    const std::string inputfile(inputfilePtr);
    mxFree(inputfilePtr); inputfilePtr = static_cast<char*>(0);
    
    try {
        RgbaInputFile file(inputfile.c_str());
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

/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#if _MSC_VER >= 1600
# define CHAR16_T wchar_t
#endif

#include <mex.h>

#include <string>



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
        
        
    }
    catch( std::exception &e ) {
        mexErrMsgIdAndTxt("OpenEXR:exception", e.what());
    }
}

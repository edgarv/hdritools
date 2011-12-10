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

#include "util.h"

#include <mex.h>

#include <string>
#include <vector>
#include <utility>
#include <cassert>


namespace
{

// Helper struct to hold Matlab pointers to matrices of the same size
struct MatricesVec
{
	mwSize M;
	mwSize N;
	std::vector<std::pair<const mxArray *, mxClassID> > data;
};




// Conversions from Matlab to C++. Return true if the conversion is successful
template <typename T>
bool toNative(const mxArray * pa, T & outValue)
{
	return false;
}


template <typename T>
void toNativeCheck(const mxArray * pa, T & outValue)
{
	if (!toNative(pa, outValue)) {
		mexErrMsgIdAndTxt("OpenEXR:IllegalConversion",
			"Illegal data conversion from Matlab to C++");
	}
}


template <>
bool toNative(const mxArray * pa, std::string & outValue)
{
	char * data = mxArrayToString(pa);
	if (data == NULL) {
		return false;
	}
	outValue = data;
	mxFree(data);
	return true;
}


template<>
bool toNative(const mxArray * pa, std::vector<std::string> & outVec)
{
	// Be tolerant and accept a single string
	if (mxIsChar(pa)) {
		std::string result;
		if (!toNative(pa, result)) {
			return false;
		}
		outVec.push_back(result);
		return true;
	}
	else if (!mxIsCell(pa)) {
		return false;
	}

	// Handle the cell array
	const mwSize numDim = mxGetNumberOfDimensions(pa);
	if (numDim != 2) {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion", "Tensors not supported.");
		return false;
	}
	const mwSize * dim = mxGetDimensions(pa);
	if (dim[0] != 1 && dim[1] != 1) {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion", "Not a cell vector.");
		return false;
	}

	const size_t numel = mxGetNumberOfElements(pa);
	for (mwIndex i = 0; i != numel; ++i) {
		std::string result;
		const mxArray * element = mxGetCell(pa, i);
		if (!toNative(element, result)) {
			return false;
		}
		outVec.push_back(result);
	}
	return true;
}


template<>
bool toNative(const mxArray * pa, std::pair<const mxArray *, mxClassID> & pair)
{
	if (mxIsNumeric(pa)) {
		if (mxIsComplex(pa)) {
			mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion",
				"Complex data is not supported.");
			return false;
		}
		pair.first  = pa;
		pair.second = mxGetClassID(pa);
		return true;
	}
	else {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion",
			"Non numeric type: %s", mxGetClassName(pa));
		return false;
	}
}



bool isMatrix(const mxArray * pa, mwSize & outM, mwSize & outN) {
	// Handle the cell array
	const mwSize numDim = mxGetNumberOfDimensions(pa);
	if (numDim != 2) {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion", "Tensors not supported.");
		return false;
	}
	const mwSize * dim = mxGetDimensions(pa);
	outM = dim[0];
	outN = dim[1];
	return true;
}


bool isVector(const mxArray * pa, mwSize & outNumel) {
	mwSize M, N;
	if (!isMatrix(pa, M, N)) {
		return false;
	}
	if (M != 1 && N != 1) {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion", "Matrices not supported.");
		return false;
	}
	outNumel = mxGetNumberOfElements(pa);
	assert(outNumel == M*N);
	return true;
}


template<>
bool toNative(const mxArray * pa, MatricesVec & outData)
{
	// If not a cell array, it might be a single matrix
	if (!mxIsCell(pa)) {
		std::pair<const mxArray *, mxClassID> pair;
		if (!toNative(pa, pair)) {
			return false;
		}
		mwSize M, N;
		if (!isMatrix(pair.first, M, N)) {
			return false;
		}
		outData.data.push_back(pair);
		outData.M = M;
		outData.N = N;
		return true;
	}

	mwSize numel = 0;
	if (!isVector(pa, numel)) {
		return false;
	} else if (numel == 0) {
		mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion", "Empty cell vector.");
		return false;
	}

	// Get the size from the first element
	{
		const mxArray * elem = mxGetCell(pa, 0);
		std::pair<const mxArray *, mxClassID> pair;
		if (!toNative(elem, pair)) {
			return false;
		}
		mwSize M, N;
		if (!isMatrix(pair.first, M, N)) {
			return false;
		}
		outData.data.push_back(pair);
		outData.M = M;
		outData.N = N;
	}

	// Iterate over the rest
	for (mwIndex i = 1; i != numel; ++i) {
		const mxArray * elem = mxGetCell(pa, i);
		std::pair<const mxArray *, mxClassID> currPair;
		if (!toNative(elem, currPair)) {
			return false;
		}
		mwSize currM, currN;
		if (!isMatrix(currPair.first, currM, currN)) {
			return false;
		}
		else if (currM != outData.M || currN != outData.N) {
			mexWarnMsgIdAndTxt("OpenEXR:IllegalConversion",
				"Inconsistent matrix sizes.");
			return false;
		}
		outData.data.push_back(currPair);
	}

	return true;
}



// Convert from a containters.Map object. This is very memory intensive since
// it will create cell arrays using dynamic Matlab memory.
bool toNative(const mxArray * pa,
	std::vector<std::string> &outNames, MatricesVec & outData)
{
	if (!mxIsClass(pa, "containers.Map")) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Not a containers.Map object.");
	}

	// Extract the cell arrays with the data and the channel names
	mxArray * map = const_cast<mxArray *>(pa);
	mxArray * isempty   = NULL;
	mxArray * namesCell = NULL;
	mxArray * dataCell  = NULL;
	if (mexCallMATLAB(1, &isempty, 1, &map, "isempty") != 0) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Could not query the map.");
	}
	else if (mxIsLogicalScalarTrue(isempty)) {
		mxDestroyArray (isempty);
		return true;
	}
	mxDestroyArray (isempty);


	if (mexCallMATLAB(1, &namesCell, 1, &map, "keys") != 0) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Could not get the map keys.");
	}
	if (mexCallMATLAB(1, &dataCell, 1, &map, "values") != 0) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Could not get the map values.");
	}

	if (toNative(namesCell, outNames) && toNative(dataCell, outData)) {
		return true;
	} else {
		mxDestroyArray (namesCell);
		mxDestroyArray (dataCell);
		return false;
	}
}




// Unify the different ways to call the function
void prepareArguments(const int nrhs, const mxArray * prhs[])
{
	int currArg = 0;

	std::string filename;
	std::string compression("piz");
	std::string pixelType("half");
	const mxArray * attribs = NULL;
	std::vector<std::string> channelNames;
	MatricesVec channelData;


	/////////// Filename, compression and pixel type //////////////////////////
	
	if (currArg >= nrhs) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Not enough arguments.");
	}
	else if (!mxIsChar(prhs[currArg])) {
		mexErrMsgIdAndTxt("OpenEXR:argument",
			"Argument %d is not a string.", currArg + 1);
	}
	else {
		toNativeCheck(prhs[currArg], filename);
		++currArg;
	}

	if (currArg >= nrhs) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Not enough arguments.");
	}
	else if (mxIsChar(prhs[currArg])) {
		toNativeCheck(prhs[currArg], compression);
		++currArg;

		if (currArg < nrhs && mxIsChar(prhs[currArg])) {
			toNativeCheck(prhs[currArg], pixelType);
			++currArg;
		}
	}


	////////////////////// Optional arguments map ////////////////////////////////

	enum DataType { UNKNOWN, MAP, NAMES_DATA };
	DataType dType = UNKNOWN;

	if (currArg >= nrhs) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Not enough arguments.");
	}
	if ((nrhs - currArg) == 3) {
		if (mxIsClass(prhs[currArg], "containers.Map")) {
			attribs = prhs[currArg];
			++currArg;
			dType = NAMES_DATA;
		}
		else {
			mexErrMsgIdAndTxt("OpenEXR:argument",
				"Expected a containers.Map handle as argument %d.", currArg);
		}
	}
	else if ((nrhs - currArg) == 2) {
		if (mxIsClass(prhs[currArg], "containers.Map")) {
			attribs = prhs[currArg];
			++currArg;
			dType = MAP;
		}
		else {
			dType = NAMES_DATA;
		}
	}
	else if ((nrhs - currArg) == 1) {
		dType = MAP;
	}
	else {
		mexErrMsgIdAndTxt("OpenEXR:argument", "%s arguments.",
			(nrhs - currArg) > 3 ? "Too many" : "Not enough");
	}
	assert(dType != UNKNOWN);


	/////////////////////////// Channel data /////////////////////////////////////

	switch(dType) {
	case MAP:
		assert(currArg == nrhs - 1);
		if (!mxIsClass(prhs[currArg], "containers.Map")) {
			mexErrMsgIdAndTxt("OpenEXR:argument",
				"Expected a containers.Map handle as last argument.");
		}
		if (!toNative(prhs[currArg], channelNames, channelData)) {
			mexErrMsgIdAndTxt("OpenEXR:argument",
				"Could not convert the channels map at argument %d.", currArg);
		}

		break;
	case NAMES_DATA:
		assert(currArg == nrhs - 2);
		if (mxIsChar(prhs[currArg])) {
			// Single channel mode
			toNativeCheck(prhs[currArg],   channelNames);
			toNativeCheck(prhs[currArg+1], channelData);
		}
		else if (mxIsCell(prhs[currArg])) {
			// Multiple channel mode
			if (!mxIsCell(prhs[currArg+1])) {
				mexErrMsgIdAndTxt("OpenEXR:argument",
					"Expected a cell vector of real matrices as last argument.");
			}
			toNativeCheck(prhs[currArg],   channelNames);
			toNativeCheck(prhs[currArg+1], channelData);
		}
		else {
			mexErrMsgIdAndTxt("OpenEXR:argument",
				"Expected a string or cell vector of strings as second to last argument.");
		}
		break;
	default:
		mexErrMsgIdAndTxt("OpenEXR:IllegalState", "Unknown channel data format");
	}

	if (channelNames.empty()) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Empty list of channel names.");
	}
	if (channelData.data.empty()) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Empty list of channel data.");
	}
	if (channelData.M < 1 || channelData.N < 1) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Invalid data size: [%d %d].",
			static_cast<int>(channelData.M), static_cast<int>(channelData.N));
	}
	if (channelNames.size() != channelData.data.size()) {
		mexErrMsgIdAndTxt("OpenEXR:argument", "Missmatch between number of "
			"provided channel names and channel data matrices.");
	}


	mexPrintf("Filename: \"%s\", compression: \"%s\", pixelType: \"%s\" Channels:\n",
		filename.c_str(), compression.c_str(), pixelType.c_str());
	for (size_t i = 0; i != channelNames.size(); ++i) {
		mexPrintf("  \"%s\"\n", channelNames[i].c_str());
	}
	mexPrintf("Channel data, %dx%d:\n", (int)channelData.M, (int)channelData.N);
	for (size_t i = 0; i != channelData.data.size(); ++i) {
		mexPrintf("  \"%s\"\n", mxGetClassName(channelData.data[i].first));
	}
}

} // namespace



void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
    pcg::mexEXRInit();

    // Check for proper number of arguments
    if (nrhs < 2) {
        mexErrMsgIdAndTxt("OpenEXR:argument", "Not enough arguments.");
    } else if (nlhs != 0) {
        mexErrMsgIdAndTxt("OpenEXR:argument", "Too many output arguments.");
    }

	prepareArguments(nrhs, prhs);
}

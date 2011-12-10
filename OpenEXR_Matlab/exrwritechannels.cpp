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

#include <ImfAttribute.h>
#include <ImfPixelType.h>
#include <ImfCompression.h>
#include <ImfOutputFile.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>

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

// Class to be queried for actual data during the OpenEXR file creation
class Data {

public:
	Data(const std::string & filename,
		 Imf::Compression compression, Imf::PixelType targetPixelType,
		 const std::vector<std::string> & channelNames,
		 const MatricesVec channelData);

	~Data();

	typedef std::pair<std::string, const float *> DataPair;

	// Write the OpenEXR file. Note that this method may throw exceptions
	void writeEXR() const;

	inline size_t size() const {
		return m_channels.size();
	}

	inline const DataPair & operator[] (size_t index) const {
		return m_channels[index];
	}

	inline const std::string & channelName (size_t index) const {
		return m_channels[index].first;
	}

	inline const float * channelData (size_t index) const {
		return m_channels[index].second;
	}

	inline size_t xStride() const {
		return m_height;
	}

	inline size_t yStride() const {
		return 1;
	}

	inline size_t width() const {
		return m_width;
	}

	inline size_t height() const {
		return m_height;
	}

	inline const std::string & filename() const {
		return m_filename;
	}

	inline Imf::Compression compression() const {
		return m_compression;
	}

	inline Imf::PixelType type() const {
		return m_type;
	}


private:
	
	const std::string m_filename;
	const Imf::Compression m_compression;
	const Imf::PixelType m_type;
	const size_t m_width;
	const size_t m_height;
	
	// Pairs of channels and a pointer to the data
	std::vector<DataPair> m_channels;

	// Matlab managed copies in single format
	std::vector<const mxArray *> m_data;

	// Data created through mexCallMATLAB
	std::vector<mxArray *> m_allocated;
};


Data::Data(const std::string & filename,
	       Imf::Compression compression, Imf::PixelType targetPixelType,
	       const std::vector<std::string> & channelNames,
	       const MatricesVec channelData) :
m_filename(filename), m_compression(compression), m_type(targetPixelType),
m_width(channelData.N), m_height(channelData.M)
{
	assert(!channelNames.empty());
	assert(channelNames.size() == channelData.data.size());

	for (size_t i = 0; i < channelNames.size(); ++i) {
		if (channelData.data[i].second != mxSINGLE_CLASS) {
			// If the type is not float, convert
			mxArray * origData   = const_cast<mxArray *>(channelData.data[i].first);
			mxArray * singleData = NULL;
			if (mexCallMATLAB(1, &singleData, 1, &origData, "single") != 0) {
				mexErrMsgTxt("Could not convert data to single.");
			}
			m_data.push_back(singleData);
			m_allocated.push_back(singleData);
		}
		else {
			// The Matlab data is already single
			m_data.push_back(channelData.data[i].first);
		}

		DataPair pair(channelNames[i], static_cast<float *>(mxGetData(m_data[i])));
		m_channels.push_back(pair);
	}
}


Data::~Data()
{
	for (size_t i = 0; i != m_allocated.size(); ++i) {
		mxDestroyArray(m_allocated[i]);
	}
}


void Data::writeEXR() const
{
	using namespace Imf;
	using namespace Imath;

	Header header(static_cast<int>(width()), static_cast<int>(height()),
		1.0f,             // aspect ratio
		V2f(0.0f, 0.0f), // screen window center,
		1.0f,            // screen window width,
		INCREASING_Y,    // line order
		compression());

	// Insert channels in the header
	for (size_t i = 0; i != size(); ++i) {
		header.channels().insert(channelName(i), Channel(type()));
	}

	OutputFile file(filename().c_str(), header);

	// Create and populate the frame buffer
	FrameBuffer frameBuffer;
	for (size_t i = 0; i != size(); ++i) {
		char * base = reinterpret_cast<char*>(const_cast<float*>(channelData(i)));
		frameBuffer.insert(channelName(i),     // name
			Slice(FLOAT,                       // type
				  base,                        // base
				  sizeof(float) * xStride(),   // xStride
				  sizeof(float) * yStride())); // yStride
	}

	file.setFrameBuffer(frameBuffer);
	file.writePixels(static_cast<int>(height()));
}




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


// Converto to a pixel type from a Matlab String
template <>
bool toNative(const mxArray * pa, Imf::PixelType & outType)
{
	char * data = mxArrayToString(pa);
	if (data == NULL) {
		return false;
	}

	bool result = true;
	if (strcmp(data, "half") == 0) {
		outType = Imf::HALF;
	}
	else if (strcmp(data, "single") == 0) {
		outType = Imf::FLOAT;
	}
	else {
		mexWarnMsgIdAndTxt("OpenEXR:argument", "Unrecognized pixel type: %s", data);
		result = false;
	}

	mxFree(data);
	return result;
}


template <>
bool toNative(const mxArray * pa, Imf::Compression & outCompression)
{
	char * data = mxArrayToString(pa);
	if (data == NULL) {
		return false;
	}

	bool result = true;
	if (strcmp(data, "none") == 0) {
		outCompression = Imf::NO_COMPRESSION;
	}
	else if (strcmp(data, "rle") == 0) {
		outCompression = Imf::RLE_COMPRESSION;
	}
	else if (strcmp(data, "zips") == 0) {
		outCompression = Imf::ZIPS_COMPRESSION;
	}
	else if (strcmp(data, "zip") == 0) {
		outCompression = Imf::ZIP_COMPRESSION;
	}
	else if (strcmp(data, "piz") == 0) {
		outCompression = Imf::PIZ_COMPRESSION;
	}
	else if (strcmp(data, "pxr24") == 0) {
		outCompression = Imf::PXR24_COMPRESSION;
	}
	else if (strcmp(data, "b44") == 0) {
		outCompression = Imf::B44_COMPRESSION;
	}
	else if (strcmp(data, "b44a") == 0) {
		outCompression = Imf::B44A_COMPRESSION;
	}
	else {
		mexWarnMsgIdAndTxt("OpenEXR:argument", "Unrecognized compression: %s", data);
		result = false;
	}

	mxFree(data);
	return result;
}




// Unify the different ways to call the function
Data * prepareArguments(const int nrhs, const mxArray * prhs[])
{
	int currArg = 0;

	std::string filename;
	Imf::Compression compression = Imf::PIZ_COMPRESSION;
	Imf::PixelType pixelType = Imf::HALF;
	
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

	if (attribs != NULL) {
		mexWarnMsgIdAndTxt("OpenEXR:unsupported",
			"Attribute support has not been implemented yet.");
	}


	mexPrintf("Filename: \"%s\", compression: \"%d\", pixelType: \"%d\" Channels:\n",
		filename.c_str(), compression, pixelType);
	for (size_t i = 0; i != channelNames.size(); ++i) {
		mexPrintf("  \"%s\"\n", channelNames[i].c_str());
	}
	mexPrintf("Channel data, %dx%d:\n", (int)channelData.M, (int)channelData.N);
	for (size_t i = 0; i != channelData.data.size(); ++i) {
		mexPrintf("  \"%s\"\n", mxGetClassName(channelData.data[i].first));
	}

	return new Data(filename, compression, pixelType, channelNames, channelData);
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

	try {
		std::auto_ptr<Data> data(prepareArguments(nrhs, prhs));
		data->writeEXR();
	}
	catch (Iex::BaseExc & e) {
		mexErrMsgIdAndTxt("OpenEXR:exception", e.what());
	}
}

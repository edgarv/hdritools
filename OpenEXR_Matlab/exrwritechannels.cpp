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
#include "ImfToMatlab.h"
#include "MatlabToImf.h"

#include <mex.h>

#include <half.h>
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


namespace pcg
{

// Helper struct to hold Matlab pointers to matrices of the same size
struct MatricesVec
{
	mwSize M;
	mwSize N;
	std::vector<std::pair<const mxArray *, mxClassID> > data;
};


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

} // namespace pcg




using namespace pcg;


namespace
{

// Class to be queried for actual data during the OpenEXR file creation
class Data {

public:
	Data(const std::string & filename,
		 Imf::Compression compression, Imf::PixelType targetPixelType,
		 const std::vector<std::string> & channelNames,
		 const MatricesVec channelData);

	~Data();

	typedef std::pair<std::string, char *> DataPair;

	// Write the OpenEXR file. Note that this method may throw exceptions
	void writeEXR() const;

	inline size_t size() const {
		return m_channels.size();
	}

	inline size_t typeSize() const {
		switch(type()) {
		case Imf::HALF:
			return sizeof(half);
			break;
		case Imf::FLOAT:
			return sizeof(float);
			break;
		default:
			assert("Unknown type" == 0);
			return 0;
		}
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

	// Helper function to create local copies of the data if necessary
	template <typename TargetType>
	char * prepareChannel(const std::pair<const mxArray *, mxClassID> & pair);

	inline const std::string & channelName (size_t index) const {
		return m_channels[index].first;
	}

	inline char * channelData (size_t index) const {
		return m_channels[index].second;
	}

	
	const std::string m_filename;
	const Imf::Compression m_compression;
	const Imf::PixelType m_type;
	const size_t m_width;
	const size_t m_height;
	
	// Pairs of channels and a pointer to the data
	std::vector<DataPair> m_channels;

	// Data created through mxMalloc
	std::vector<void *> m_allocated;
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
		char * data;
		switch (type()) {
		case Imf::FLOAT:
			data = prepareChannel<float>(channelData.data[i]);
			break;
		case Imf::HALF:
			data = prepareChannel<half>(channelData.data[i]);
			break;
		default:
			assert("Unsupported Pixel Type" == 0);
			mexErrMsgIdAndTxt("OpenEXR:unsupported",
				"Unsupported pixel type: %d", static_cast<int>(type()));
			data = NULL; // Keep compiler happy

		}

		DataPair pair(channelNames[i], data);
		m_channels.push_back(pair);
	}
}


template <typename TargetType>
char * Data::prepareChannel(const std::pair<const mxArray *, mxClassID> & pair)
{
	const mxArray* pa       = pair.first;
	const mxClassID srcType = pair.second;

	if (srcType == pcg::mex_traits<TargetType>::classID) {
		// If the type is compatible, just return a pointer to the Matlab data
		return static_cast<char*>(mxGetData(pa));
	}
	else {
		// Allocate enough space and convert in place
		const size_t numPixels = width() * height();
		void * rawData = mxMalloc(sizeof(TargetType) * numPixels);
		m_allocated.push_back(rawData);
		TargetType * dest = static_cast<TargetType*> (rawData);
		convertData(dest, pa, srcType, numPixels);
		return reinterpret_cast<char*>(dest);
	}
}


Data::~Data()
{
	for (size_t i = 0; i != m_allocated.size(); ++i) {
		mxFree(m_allocated[i]);
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
		frameBuffer.insert(channelName(i),  // name
			Slice(type(),                   // type
				  channelData(i),           // base
				  typeSize() * xStride(),   // xStride
				  typeSize() * yStride())); // yStride
	}

	file.setFrameBuffer(frameBuffer);
	file.writePixels(static_cast<int>(height()));
}







using namespace pcg;




// Unify the different ways to call the function
Data * prepareArguments(const int nrhs, const mxArray * prhs[])
{
	int currArg = 0;

	std::string filename;
	Imf::Compression compression = Imf::ZIP_COMPRESSION;
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


	// B44[a] only compresses half channels
	if (pixelType != Imf::HALF && (compression == Imf::B44_COMPRESSION || 
		                           compression == Imf::B44A_COMPRESSION))
	{
		mexWarnMsgIdAndTxt("OpenEXR:compression",
			"B44[A] format stores uncompressed data when the pixel type is not HALF.");
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

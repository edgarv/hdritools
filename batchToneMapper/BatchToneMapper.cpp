#include "BatchToneMapper.h"

#include <iostream>

// Misc utitilities
#include "Util.h"

// Pipeline filters
#include "FileInputFilter.h"
#include "ZipfileInputFilter.h"
#include "ToneMappingFilter.h"

using namespace std;


string BatchToneMapper::defaultFormat;


BatchToneMapper::BatchToneMapper(const vector<string>& files, bool bpp16) :
toneMapper(LUT_SIZE), offset(0), tokens(0), useBpp16(bpp16),
format(!bpp16 ? getDefaultFormat() : "png")
{
	classifyFiles(files);

	// Runs the pipeline with 2.5x the number of working threads
	tokens = (int)(2.5f * Util::numberOfProcessors());
	assert(tokens > 0);
}

void BatchToneMapper::setupToneMapper(float exposure, float gamma) {
	toneMapper.SetExposure(exposure);
	toneMapper.SetGamma(gamma);
	toneMapper.SetSRGB(false);
}

void BatchToneMapper::setupToneMapper(float exposure) {
	toneMapper.SetExposure(exposure);
	toneMapper.SetSRGB(true);
}

void BatchToneMapper::setFormat(const string & newFormat)
{
	if (!useBpp16) {
		// There are not many formats, so this is not that bad
		const vector<string> & formats = Util::supportedWriteImageFormats();
		for(vector<string>::const_iterator it = formats.begin();
			it != formats.end(); ++it)
		{
			if (newFormat == *it) {
				format = newFormat;
				return;
			}
		}
		cerr << "Warning: unsupported format \"" << newFormat << "\"" << endl;
	}
	else {
		// Only png is supported
		if (newFormat != "png") {
			cerr << "Warning: unsupported format for bpp16 \"" << newFormat 
				 << "\". Using png." << endl;
			format = "png";
		}
	}
}


void BatchToneMapper::execute() const {

	if (zipFiles.size() > 0) {
		executeZip();
		cout << "All Zip files have been processed." << endl;
	}

	if (hdrFiles.size() > 0) {
		executeHdr();
		cout << "All HDR files have been processed." << endl;
	}
}

const string & BatchToneMapper::getDefaultFormat() {

	if (defaultFormat.empty()) {

		// Default to png, jpg, or whatever is first
		if (Util::isPngSupported()) {
			defaultFormat = "png";
		}
		else if (Util::isJpgSupported()) {
			defaultFormat = "jpg";
		}
		else {
			defaultFormat = (Util::supportedWriteImageFormats())[0];
		}
	}

	return defaultFormat;
}

void BatchToneMapper::classifyFiles(const vector<string> & files) {

	for(vector<string>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		bool isZip;
		bool isHdr;
		const bool readable = Util::isReadable(*it, isZip, isHdr);

		if (!readable) {
			cerr << "Warning: cannot read " << *it << "." << endl;
		}
		else {
			if (isZip) {
				zipFiles.push_back(*it);
			}
			else if (isHdr) {
				hdrFiles.push_back(*it);
			}
			else {
				cerr << "Warning: the file " << *it << " doesn't have a recognized type." << endl;
			}
		}
	}
}

void BatchToneMapper::executeZip() const {
	
	// Creates and uses a TBB pipeline
	tbb::pipeline pipeline;

	// Adds the zip-reading filter
	ZipfileInputFilter zipFilter(zipFiles, format, offset);
	pipeline.add_filter(zipFilter);

	// Adds the tone mapping filter
	ToneMappingFilter toneFilter(toneMapper, useBpp16);
	pipeline.add_filter(toneFilter);

	
	pipeline.run(tokens);

	// Clears the filters after it's done
	pipeline.clear();
}


void BatchToneMapper::executeHdr() const {

	// Creates and uses a TBB pipeline
	tbb::pipeline pipeline;

	// Adds the file input and loading filters
	FileInputFilter  inputFilter(hdrFiles);
	FileLoaderFilter loaderFilter(format, offset);
	
	pipeline.add_filter(inputFilter);
	pipeline.add_filter(loaderFilter);

	// Adds the tone mapping filter
	ToneMappingFilter toneFilter(toneMapper, useBpp16);
	pipeline.add_filter(toneFilter);

	pipeline.run(tokens);

	// Clears the filters after it's done
	pipeline.clear();
}


ostream& operator<<(ostream& os, const BatchToneMapper& b)
{
	os << "BatchToneMapper: LUT size " << BatchToneMapper::LUT_SIZE 
	   << ", using " << b.tokens << " pipeline tokens." << endl
	   << "Conversion parameters:" << endl
	   << "  Exposure:  " << b.toneMapper.Exposure() << endl
	   << "  Gamma:     " ;

	if ( b.toneMapper.isSRGB() ) {
		os << "NA (using sRGB)" << endl;
	}
	else {
	   os << b.toneMapper.Gamma() << endl;
	}

	os << "  Offset:    " << b.offset << endl
	   << "  BPP:       " << (b.useBpp16 ? 16 : 8) << endl
	   << "  Format:    " << b.format << endl;
	if(b.zipFiles.size() > 0) {
		os << "  Zip Files: ";
		for (vector<string>::const_iterator it = b.zipFiles.begin(); 
			 it != b.zipFiles.end(); ++it) {
			os << *it << ' ';
		}
		os << endl;
	}
	if(b.hdrFiles.size() > 0) {
		os << "  HDR Files: ";
		for (vector<string>::const_iterator it = b.hdrFiles.begin();
			 it != b.hdrFiles.end(); ++it) {
			os << *it << ' ';
		}
		os << endl;
	}

	return os;
}

#include "FileInputFilter.h"
#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <fstream>
#include <iostream>

using tbb::filter;

using std::vector;
using std::string;
using std::ifstream;
using std::cerr;
using std::ios_base;


FileInputFilter::FileInputFilter(const vector<string> &fileNames) :
	filter(/*is_serial*/ true),
	files(fileNames)
{
	filename = files.begin();
}

void* FileInputFilter::operator()(void*)
{
	// Super simple!
	if (filename != files.end()) {
		const char * filenamePtr = filename->c_str();
		++filename;
		return const_cast<char*>(filenamePtr);
	}
	else {
		return NULL;
	}
}



FileLoaderFilter::FileLoaderFilter(const std::string &format, int filenameOffset) :
  tbb::filter(/*is_serial*/false),
  formatStr(format.c_str()),
  offset(filenameOffset) 
{}


void* FileLoaderFilter::operator()(void* arg)
{
	const char *filename = static_cast<const char*>(arg);

	// Opens the input stream
	ifstream is(filename, ios_base::binary);
	if (! is.bad() ) {
		return FloatImageProcessor::load(filename, is, formatStr, offset);
	}
	else {
		cerr << "Ooops! Unable to open " << filename << " for reading.";
		return new ImageInfo;
	}

}

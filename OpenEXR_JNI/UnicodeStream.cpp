#include "UnicodeStream.h"

#include <Iex.h>
#include <stdio.h>

#include <wchar.h>


void UnicodeStream::openFile(const wchar_t *mode) 
{
	// Use the secure CRT in MSVC 2005 and forward
#if _MSC_VER >= 1400
	errno_t err = _wfopen_s(&file, filenameStr.c_str(), mode);
	if (err != 0) {
		file = NULL;
		Iex::throwErrnoExc();
	}
#else
	file = _wfopen(filenameStr.c_str(), mode);
	if (file == NULL) {
		Iex::throwErrnoExc();
	}
#endif
}


void UnicodeStream::setFilename(const jchar *filename, const jint length) 
{
	// Just die in this (unlikely) case
	if (sizeof(jchar) > sizeof(wchar_t)) {
		throw Iex::LogicExc("jchar is larger than wchar_t");
	}
	// Easy case, this is true for example in Windows
	if (sizeof(jchar) == sizeof(wchar_t)) {
		filenameStr = reinterpret_cast<const wchar_t*>(filename);
	}
	// Unix case: wchar_t is for UCS-4 stuff, and the jchar is always for UCS-2
	else {
		filenameStr.resize(length, '\0');
		for(int i = 0; i < length; ++i) {
			filenameStr[i] = filename[i];
		}
	}
}

UnicodeStream::~UnicodeStream () {
	if (file != NULL) {
		errno_t err = fclose(file);
	}
}

Imath::Int64 UnicodeStream::tell()
{
	if (file != NULL) {
		return _ftelli64(file);
	}
	else {
		throw Iex::IoExc();
	}
}

void UnicodeStream::seek(Imath::Int64 pos)
{
	if( _fseeki64(file, pos, SEEK_SET) != 0 ) {
		Iex::throwErrnoExc();
	}
}


// ###### UnicodeIStream ######

bool UnicodeIFStream::read  (char c[/*n*/], int n)
{
	if (c == NULL) {
		throw Iex::InputExc ("NULL buffer.");
	}
	size_t numRead = fread(c, 1, n, file);
	if (numRead != n) {
		if(feof(file)) {
			return false;
		}
		else {
			Iex::throwErrnoExc();
		}
	}
	return true;
}

void UnicodeIFStream::clear ()
{
	if (file == NULL) {
		throw Iex::IoExc();
	}

#if _MSC_VER >= 1400
	if ( clearerr_s(file) != 0 ) {
		Iex::throwErrnoExc();
	}
#else
	clearerr(file);
#endif
}

// ###### UnicodeOStream ######

void UnicodeOFStream::write (const char c[/*n*/], int n)
{
	if (c == NULL) {
		throw Iex::InputExc ("NULL buffer.");
	}
	size_t numWritten = fwrite(c, 1, n, file);
	if (numWritten != n) {
		Iex::throwErrnoExc();
	}
}

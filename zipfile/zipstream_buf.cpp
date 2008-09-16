#include "zipstream_buf.h"
#include "unzip.h"

#include <cassert>
#include <iostream>
#include <algorithm>

using namespace pcg;
using std::min;
using std::cerr;
using std::endl;


const zipstream_buf::pos_type zipstream_buf::BAD_POSITION = 
	zipstream_buf::pos_type(zipstream_buf::off_type(-1));

zipstream_buf::zipstream_buf(const void* _m_uzFile, const unsigned long _size) :
	m_uzFile(_m_uzFile), size(_size), readBytes(0), endOfEntry(false)
{
	if (m_uzFile == NULL) {
		throw std::exception("Null unzip archive");
	}

	if (unzOpenCurrentFile(const_cast<void*>(m_uzFile)) != UNZ_OK) {
		throw std::exception("Error when opening the current zip file");
	}

	// Set the pointers to the beginning  of the stuff,
	// the get next and the end pointer
	// Initially the get pointer is at the end so that
	// it fills the buffer upon the first request for data
	setg(&buffer[0], &buffer[BUF_SIZE], &buffer[BUF_SIZE]);
}

zipstream_buf::int_type	zipstream_buf::underflow() 
{
	// You've read everything: don't even think about reading more!
	if (endOfEntry) {
		return traits_type::eof();
	}

	// Ok, perhaps we can just happily return whatever is in the buffer
	if (gptr() != egptr()) {
		// Check for evil modifications in debug mode
		assert(gptr() >= &buffer[0] && gptr() < &buffer[BUF_SIZE]);
		return traits_type::not_eof( *gptr() );
	}

	// This case means that we have to fill the buffer with more data,
	// update the pointers and updates the readBytes variable
	int nRet = unzReadCurrentFile(const_cast<void*>(m_uzFile), buffer, BUF_SIZE);
	if (nRet > 0) {
		assert(nRet <= BUF_SIZE);
		setg(&buffer[0], &buffer[0], &buffer[nRet]);
		readBytes += nRet;
		return traits_type::not_eof( *gptr() );
	}
	else {
		// That's it, whe read everything, did we?
		endOfEntry = true;
		if (readBytes != size) {
			throw std::exception("Unexpected end of zip entry");
		}
		if (unzCloseCurrentFile(const_cast<void*>(m_uzFile)) == UNZ_CRCERROR) {
			throw std::exception("Read all the file but the CRC is not good");
		}
		return traits_type::eof();
	}
}

streamsize zipstream_buf::showmanyc() {

	if (endOfEntry) {
		return 0;
	}
	else {
		return min(BUF_SIZE, static_cast<int>(size-readBytes));
	}
}

// WARNING: This hasn't been tested throughly! there might be horrible bugs!
zipstream_buf::pos_type zipstream_buf::seekpos(pos_type sp, ios_base::openmode which)
{
	// If it's utterly invalud just return
	const int buffBase = ((readBytes - 
		min(BUF_SIZE, static_cast<int>(size-readBytes)))/BUF_SIZE)*BUF_SIZE;
	if (sp < 0 || sp > size || sp < buffBase || sp > (buffBase+BUF_SIZE)) {
		return BAD_POSITION;
	}

	// Lets limit the seek to within the buffer
	const int relativePos = static_cast<int>(sp < BUF_SIZE ? sp : sp % BUF_SIZE);
	assert(relativePos >= 0 && relativePos < BUF_SIZE);

	setg(&buffer[0], &buffer[relativePos], egptr());

	return sp;
}


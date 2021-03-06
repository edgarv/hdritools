/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

// This is the class that will do the actual magic behind scenes.
// This guy then be encapsulated into a generic istream: we don't
// want to expose this to the public... should we though?

#ifndef PCG_ZIPSTREAM_BUF_H
#define PCG_ZIPSTREAM_BUF_H

#include "unzip.h"
#include <streambuf>

using std::streamsize;
using std::streambuf;
using std::ios_base;

#define BUF_SIZE 8192

namespace pcg {

	// Forward declaration
	class ZipFile;

	class zipstream_buf : public streambuf {

		friend class ZipFile;

	private:
		// Basic exception type
		class ZipException: public std::exception
		{
		  public:
			ZipException (const char* text=0) throw()     : message(text) {}
			ZipException (const std::string &text) throw(): message(text) {}

			virtual ~ZipException() throw () {}

			virtual const char * what () const throw () {
				return message.c_str();
			}

		private:
			std::string message;
		};

	protected:
		char buffer[BUF_SIZE];

		const static pos_type BAD_POSITION;

		// Pointer to the underlying zip file from the original zip library
		const void* m_uzFile;

		// The uncompressed size of the file
		const unsigned int size;

		// Count of the bytes read so far from the file
		unsigned int readBytes;

		// A flag to remember when we reach the end of the entry
		bool endOfEntry;

		// Yes, the constructor is protected: this class depends completely in the ZipFile
		// for a proper behavior
		zipstream_buf(const void* _uzFile, const unsigned long _size);

		// This is the trully magic function which fills the buffer
		// and manipulates the current character pointer
		virtual int_type underflow();

		// Let's be nice and announce how many bytes can be read
		// before unzipping more stuff
		virtual streamsize showmanyc();

		virtual int overflow(int c) {
			return streambuf::overflow(c);
		}

		// Tries to alter the current positions for the controlled streams
		virtual pos_type seekpos(pos_type _Sp,
			ios_base::openmode _Which = ios_base::in | ios_base::out);

	};


}


#endif /* PCG_ZIPSTREAM_BUF_H */

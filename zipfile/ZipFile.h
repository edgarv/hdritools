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

#ifndef PCG_ZIPFILE_H
#define PCG_ZIPFILE_H


#include <istream>
#include <vector>

#include <string>

using std::istream;
using std::vector;
using std::string;

namespace pcg {

	/* Forward declaration */
	class zipstream_buf;

	/**
	 * An entry in the zip file. This class is NOT thread safe!!!
	 */
	class ZipEntry {

	protected:
		string name;
		string comment;
		unsigned int index;
		unsigned long compressedSize;
		unsigned long crc;
		int method;
		unsigned long size;
		unsigned long time;
		bool isDirectory;


		// The one constructor
		ZipEntry(unsigned int index = 0) : 
		  compressedSize(0), crc(0),
		  method(0), size(0), time(0), 
		  isDirectory(false)
		{
			this->index = index;
		}

		// This this does a deep copy
		ZipEntry(const ZipEntry& entry) 
		{
			name = entry.name;
			comment = entry.comment;
			index = entry.index;
			compressedSize = entry.compressedSize;
			crc = entry.crc;
			method = entry.method;
			size = entry.size;
			time = entry.time;
			isDirectory = entry.isDirectory;
		}

		// Returns the index of this entry in the zipfile
		inline unsigned int GetIndex() const {
			return index;
		}

	public:

		friend class ZipFile;

		// Returns the comment string for the entry, or null if none
		inline const char* GetComment() const {
			return comment.c_str();
		}

		// Returns the size of the compressed data
		inline unsigned long GetCompressedSize() const {
			return compressedSize;
		}

		// Returns the CRC32 checksum of the uncompressed data
		inline unsigned long GetCrc() const {
			return crc;
		}

		// Returns the compression method of the entry
		inline int GetMethod() const {
			return method;
		}

		// Returns the name of the entry
		inline const char* GetName() const {
			return name.c_str();
		}

		// Returns the uncompressed size of the entry data
		inline unsigned long GetSize() const {
			return size;
		}

		// Returns the modification time of the entry
		inline unsigned long GetTime() const {
			return time;
		}

		// Returns true if this is a directory entry
		inline bool IsDirectory() const {
			return isDirectory;
		}

	};


	// A useful typedef
	typedef vector<ZipEntry*> ZipEntryVector;


	/**
	 * A small class for reading zipfile, done in the spirit of Java's ZipFile
	 */
	class ZipFile {

	protected:

		// The internal unzip handle
		void *m_uzFile;

		// The number of entries
		unsigned int numEntries;

		// A small state var
		bool isOpen;

		// Index of the current file
		unsigned int currIndex;

		// Moves to the first file in the zip file
		bool GotoFirstFile();

		// Moves to the next file in the zip file
		bool GotoNextFile();

		// Moves the the specified index in the file (0-based!)
		bool GotoFile(unsigned int nFile);

		// Reads the next ZIP file entry, or null if there are no more entries
		ZipEntry* GetNextEntry();

		// A vector with the poiner to all entries
		ZipEntryVector entries;

		// The helper istream used for all entries
		// Remember that you can be reading from only one entry at a time!
		istream *zipstream;

		// The currently active zipstream_buf
		zipstream_buf *zstreambuf;

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
		
	public:

		// Another usedful typedefs
		typedef ZipEntryVector::const_iterator const_iterator;


		// The fancy destructor
		~ZipFile();

		// Utility constructor to avoid creating the stream
		ZipFile(const char *name);

		// Closes the ZIP file
		void close();

		// Returns an input stream for reading the contents of the specified zip file
		// entry. This method is totally thread UNSAFE, as only one of this streams
		// can be active per file at the same time. So use with care!
		istream& GetInputStream(const ZipEntry *entry);


		// #########  INLINE METHODS #############

		// Returns the number of entries in the ZIP file
		inline unsigned int size() const {
			return numEntries;
		}

		// Iterator to the begining of the entries
		inline const_iterator begin() {
			return entries.begin();
		}

		// Iterator to the end of the entries
		inline const_iterator end() {
			return entries.end();
		}

	};


}


#endif /* PCG_ZIPFILE_H */

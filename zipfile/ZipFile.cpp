#include "ZipFile.h"
#include "zipstream_buf.h"

#include "unzip.h"

#include <cassert>
#include <iostream>


using namespace pcg;
using std::cerr;
using std::endl;

ZipFile::ZipFile(const char *name) :
	m_uzFile(NULL), isOpen(false), currIndex(0),
	zipstream(NULL), zstreambuf(NULL)
{

	if (name == NULL) {
		throw std::exception("Null pointer to name");
	}

	m_uzFile = unzOpen(name);
	if (m_uzFile == NULL) {
		throw std::exception("Error openning zipfile");
	}

	unz_global_info info;

	// Gets the global info
	if (unzGetGlobalInfo(m_uzFile, &info) != UNZ_OK)
	{
		throw std::exception("Couln't get the global info");
	}

	isOpen = true;
	numEntries = info.number_entry;

	// Loads all the entries in the vector
	ZipEntry *entry;
	while ( (entry = GetNextEntry()) != NULL ) {
		entries.push_back(entry);
	}

}

ZipFile::~ZipFile() {
	close();
}

void ZipFile::close() {

	if (isOpen) {
		unzCloseCurrentFile(m_uzFile);

		int nRet = unzClose(m_uzFile);
		m_uzFile = NULL;

		if (zstreambuf != NULL) {
			delete zstreambuf;
		}
		if (zipstream != NULL) {
			delete zipstream;
		}

		// Let's kill the entries
		for (unsigned int i = 0; i < entries.size(); ++i) {
			delete entries[i];
		}
		entries.clear();

		isOpen = false;
	}
}

bool ZipFile::GotoFirstFile()
{
	if (m_uzFile == NULL) {
		return false;
	}
	else {
		currIndex = 1;
		return (unzGoToFirstFile(m_uzFile) == UNZ_OK);
	}
}

bool ZipFile::GotoNextFile()
{
	if (m_uzFile == NULL) {
		return false;
	}
	else {
		++currIndex;
		return (unzGoToNextFile(m_uzFile) == UNZ_OK);
	}
}


bool ZipFile::GotoFile(unsigned int nFile)
{
	if (m_uzFile == NULL)
		return false;

	if (nFile >= size())
		return false;

	// If we are already there, do nothing
	if (nFile + 1 == currIndex) {
		return true;
	}

	unsigned int count;

	// If it's before our current position, we have to go back to the beginning
	if (nFile+1 < currIndex) {
		GotoFirstFile();
		count = nFile;
	}
	// Otherwise we need less work!
	else {
		count = nFile+1-currIndex;
	}

	while (count--)
	{
		if (!GotoNextFile())
			return false;
	}

	return true;
}


ZipEntry* ZipFile::GetNextEntry()
{
	if (!isOpen) {
		throw std::exception("Invalid state: the file is not open");
	}
	else if (size() == currIndex) {
		return NULL;	// No more entries
	}

	// A sanity check
	assert( m_uzFile != NULL );

	bool succeeded = currIndex == 0 ? GotoFirstFile() : GotoNextFile();
	if (!succeeded) {
		cerr << "Error: couldn't get the next zip entry even though it was expected" << endl;
		return NULL;
	}

	// After being in the correct position, get the file info
	unz_file_info uzfi;
	memset(&uzfi, 0, sizeof(uzfi) );
	char filename[512];
	char comment[512];
	const int FILE_ATTRIBUTE_DIRECTORY = 0x10;

	if (UNZ_OK != unzGetCurrentFileInfo(m_uzFile, &uzfi, filename, sizeof(filename), 
		NULL, 0, comment, sizeof(comment) )) {
		cerr << "Error: couldn't get the next zip entry information!" << endl;
		return NULL;
	}

	// copy across. Account for the the fact that currIndex is 1-based
	ZipEntry *entry = new ZipEntry(currIndex-1);
	entry->comment  = comment;
	entry->name     = filename;

	entry->compressedSize = uzfi.compressed_size;
	entry->crc = uzfi.crc;
	entry->method = uzfi.compression_method;
	entry->size = uzfi.uncompressed_size;
	entry->time = uzfi.dosDate;
	entry->isDirectory = (uzfi.external_fa & FILE_ATTRIBUTE_DIRECTORY) == 
		                 FILE_ATTRIBUTE_DIRECTORY;

	return entry;
}

istream& ZipFile::GetInputStream(const ZipEntry *entry) {

	if (!isOpen) {
		throw std::exception("Invalid state: the file is not open");
	}

	// We validate that the entry matches what we have
	const ZipEntry *e = entries.at(entry->GetIndex());
	if (e->GetCrc() != entry->GetCrc()) {
		throw std::exception("Entries missmatch, are you sure this is an entry from this zip?");
	}

	// Once the entry has been validated, we need to position the zipfile at that entry
	if ( !GotoFile(e->GetIndex()) ) {
		throw std::exception("Unexpeced error when moving to the specified file index");
	}
	zipstream_buf *zipbuffer = new zipstream_buf(m_uzFile, e->GetSize());

	// With the corresponding buffer created we just adjust the state variables and return
	if (zipstream == NULL) {
		zipstream = new istream(zipbuffer);
	}
	else {
		zipstream->rdbuf(zipbuffer);
	}
	if (zstreambuf != NULL) {
		delete zstreambuf;
	}
	zstreambuf = zipbuffer;

	return *zipstream;

}

/** Classes to create OpenEXR streams which open wide-character filenames */

#if !defined(UNICODE_STREAM_H)
#define UNICODE_STREAM_H

#include <ImfIO.h>
#include <string>
#include <jni.h>

// A base helper class for handling wchar_t/jchar arguments.
// It's based on standard FILE* functions, and provides the
// basic elements for setting up the base parameters, closing
// the file descriptor on exit and tell/seek functions.
class UnicodeStream {

protected:
	FILE *file;
	std::wstring filenameStr;

	// Opens the file using the given mode (as defined in wfopen())
	// using the filename stored in filenameStr
	void openFile(const wchar_t *mode);

	// Utility function to set the internal filename from a jchar string
	// of the given lenght (aka number of (jchar) characters).
	void setFilename(const jchar *filename, const jint length);

public:
	UnicodeStream() : file(NULL), filenameStr(L"")
	{}

	UnicodeStream(const wchar_t *filename) : file(NULL), filenameStr(filename)
	{}

	UnicodeStream(const std::wstring & filename) : file(NULL), filenameStr(filename)
	{}

	virtual ~UnicodeStream ();

	// Current position in the file
	Imath::Int64 tell();

	// Offset from the beginning
	void seek(Imath::Int64 pos);

	const wchar_t * wfilename() const {
		return filenameStr.c_str();
	}
};





class UnicodeIFStream : public Imf::IStream, protected UnicodeStream {

public:

	UnicodeIFStream(const wchar_t * filename) : Imf::IStream("Unicode istream"),
		UnicodeStream(filename)
	{
		openFile(L"rb");
	}
	UnicodeIFStream(const std::wstring & filename) : Imf::IStream("Unicode istream"),
		UnicodeStream(filename)
	{
		openFile(L"rb");
	}
	UnicodeIFStream(const jchar *filename, const jint length) : Imf::IStream("Unicode istream")
	{
		setFilename(filename, length);
		openFile(L"rb");
	}

    virtual bool read  (char c[/*n*/], int n);
    
	virtual Imath::Int64	tellg ()
	{
		return tell();
	}

	virtual void seekg (Imath::Int64 pos)
	{
		seek(pos);
	}

    virtual void clear ();
};





class UnicodeOFStream : public Imf::OStream, protected UnicodeStream {

public:

	UnicodeOFStream (const wchar_t *filename) : Imf::OStream("Unicode ostream"),
		UnicodeStream(filename)
	{
		openFile(L"wb");
	}
	UnicodeOFStream (const std::wstring & filename) : Imf::OStream("Unicode ostream"),
		UnicodeStream(filename)
	{
		openFile(L"wb");
	}
	UnicodeOFStream(const jchar *filename, const jint length) : Imf::OStream("Unicode Ostream")
	{
		setFilename(filename, length);
		openFile(L"wb");
	}


    virtual void write (const char c[/*n*/], int n);

	virtual Imath::Int64	tellp ()
	{
		return tell();
	}
    
	virtual void seekp (Imath::Int64 pos)
	{
		seek(pos);
	}

};


#endif /* UNICODE_STREAM_H */

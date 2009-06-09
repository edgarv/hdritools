#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <iostream>
#include <istream>
#include <Rgba32F.h>
#include <Image.h>
#include <RgbeIO.h>
#include <OpenEXRIO.h>
#include <PfmIO.h>

#include <QString>
#include <QRegExp>


ImageInfo* FloatImageProcessor::load(const char *filenameStr, std::istream &is, 
									 const char *formatStr, int offset)
{
	// Creates the used regular expressions
	QRegExp rgbeRegex(".+\\.(rgbe|hdr)$", Qt::CaseInsensitive);
	QRegExp exrRegex(".+\\.exr$", Qt::CaseInsensitive);
	QRegExp pfmRegex(".+\\.pfm$", Qt::CaseInsensitive);

	// Pointer with the result image
	Image<Rgba32F> *floatImage = new Image<Rgba32F>();

	// Get the QString version of the filename
	QString filename(filenameStr);

	// Tries to find the type of image based on the extension
	try {
		if (rgbeRegex.exactMatch(filename)) {

			// Creates the RGBE Image from the stream
			RgbeIO::Load(*floatImage, is);
		}
		else if (exrRegex.exactMatch(filename)) {

			// Loads the OpenEXR file
			OpenEXRIO::Load(*floatImage, is);
		}
		else if (pfmRegex.exactMatch(filename)) {

			// Loads the Pfm file
			PfmIO::Load(*floatImage, is);
		}
		else {
			cerr << "Ooops! Unrecognized file : " << filenameStr << endl;
			// Returns an invalid handle
			return new ImageInfo;
		}
	}
	catch (std::exception e) {
		cerr << "Ooops! While loading " << filenameStr << ": " << e.what() << endl;
		return new ImageInfo;
	}

	assert(floatImage->Height() > 0 && floatImage->Width() > 0);

	// Gets the output name
	setTargetName(filename, formatStr, offset);

	// The data is ready for the next stage, just return it
	ImageInfo *info = new ImageInfo(floatImage, 
		string(filenameStr), filename.toStdString());
	return info;

}

void FloatImageProcessor::setTargetName(QString & filename, 
										const char *formatStr, int offset)
{
	QRegExp trailingDigit("(\\d+)\\.(\\w+)$");
	int pos = -1;
	if(offset != 0 && (pos = trailingDigit.indexIn(filename)) > -1) {
		
		// cap(1) corresponds to (\d+) : a sequence of digits
		QString numValue = trailingDigit.cap(1);
		// By construction this must yield a valid number
		const int val = numValue.toInt() + offset;
		const int fieldWidth = numValue.length();
		// Replace the trailing number with the appropriate padding while keeping
		// the original extension and what was before the number.
		filename.replace(pos, trailingDigit.matchedLength(), QString("%1.%2")
			.arg(val, fieldWidth, /*base=*/10, /*fillChar=*/ QLatin1Char('0'))
			.arg(formatStr) );
	}
	else {
		// Create the output filename just by replacing the extension
		QRegExp extRegex("\\.\\w+$");
		filename.replace( extRegex, QString(".%1").arg(formatStr) );
	}

}

// Main entry point for the console application
//
// Batch tonemapper. It reads the HDR files and all the images within the given zip files and
// tonemaps them using the specified parameters (gamma, exposure compensation and format).
// When the input filename contains a trailing number (i.e. file-0001.exr, 00003.rgbe) 
// an offset may be applied to generate the output file (i.e. with an offset=10; 
// file-0001.exr -> file-0011.png, 00003.rgbe -> 00013.png).
// The HDR supported formats are RGBE (.rgbe, .hdr) and OpenEXR (.exr).
//
// TODO:
//   - Check/implement support for zip files with directories.
//

#include <iostream>

#include <tclap/CmdLine.h>
#include <tbb/tick_count.h>

// Main working entity
#include "BatchToneMapper.h"

// To get the list of formats
#include "Util.h"

#include <QCoreApplication>

using namespace std;
using namespace TCLAP;

using tbb::tick_count;


// Required when building with the static Qt
#if QT_STATICPLUGIN

#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
Q_IMPORT_PLUGIN(qmng)
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qtiff)

#endif /* QT_STATICPLUGIN */


// Basically we have one constraint: for the exposure multiplier and gamma we want them to be greater than zero
class ConstraintGreaterThan : public Constraint<float> {

	const float minVal;
	string desc;
	const string short_id;

public:
	ConstraintGreaterThan(float mininum) : minVal(mininum), short_id("greaterThan") {
		ostringstream s;
		s << "The value must be greater than " << minVal << ".";
		desc = s.str();
	}

	virtual string description() const {
		return desc;
	}
	virtual string shortID() const {
		return short_id;
	}
	virtual bool check(const float &value) const {
		return value > minVal;
	}

};



// Main parameter processing through TCLAP
void parseArgs(int argc, char* argv[], float &exposure, bool &srgb, float &gamma, int &offset, 
			   string &format, vector<string> &files) 
{
	try {

		CmdLine cmdline("Tone maps all the HDR images from the input files "
			" and those within the given zip files.", ' ', "0.0.2");
		ConstraintGreaterThan constraint(0.0f);

		// Exposure
		ValueArg<float> exposureArg("e", "exposure", 
			"Exponent for the exposure compensation. "
			"The pixels will be multiplied by 2^exposure prior to gamma correction (default 0).", 
			false, 0.0f, "float");

		// Exposure factor
		ValueArg<float> exposureMultiplierArg("m", "expmult",
			"Exposure compensation multiplier. "
			"The pixels will be multiplied by this value prior to gamma correction (default 1).",
			false, 1.0f, "possitive float", &constraint);

		// Gamma value
		ValueArg<float> gammaArg("g", "gamma",
			"Gamma correction. "
			"The pixels will be raised to the power of 1/gamma after the exposure compensation (default 2.2).",
			false, 2.2f, "possitive float", &constraint);

		// sRGB flag
		TCLAP::SwitchArg srgbArg("",
			"srgb",
			"The pixels will be transformed according to the sRGB space after the exposure compensation.",
			false);

		// Offset to add to the numeric filenames
		ValueArg<int> offsetArg("o", "offset",
			"Filename offset. "
			"If the filename contains a trailing number, this value will be added to it for each final output file.",
			false, 0, "integer");

		// Output format constraint
		vector<string> supportedFormats = 
			Util::supportedWriteImageFormats();
		ValuesConstraint<string> formatConstraint(supportedFormats);
		string defaultFormat = BatchToneMapper::getDefaultFormat();

		string formatArgDesc("Image format for the tone mapped images. It must be one of: ");
		formatArgDesc += formatConstraint.description() + ". The default is " + defaultFormat + ".";
		ValueArg<string> formatArg("f", "format",
			formatArgDesc, 
			false, defaultFormat, "string", &formatConstraint);

		// The unlabeled multiple arguments are the input zipfiles
		UnlabeledMultiArg<string> filesArg("filenames", 
			"HDR images (rgbe|hdr|exr) and Zip files with HDR images to tone map.", 
			true, "filename");

		// Adds the arguments to the command line (the unlabeled multi args must be the last ones!!)
		cmdline.xorAdd(exposureArg, exposureMultiplierArg);
		cmdline.xorAdd(srgbArg, gammaArg);
		cmdline.add(offsetArg);
		cmdline.add(formatArg);
		cmdline.add(filesArg);
		
		// Parse the argv array
		cmdline.parse(argc, argv);

		// If we are here we are safe to recover the values
		srgb = srgbArg.getValue();
		gamma = gammaArg.getValue();
		if (exposureArg.isSet()) {
			exposure = exposureArg.getValue();
		}
		else if (exposureMultiplierArg.isSet()) {
			// Yes, is not that accurate but for our purposes it's enough
			exposure = log(exposureMultiplierArg.getValue())/log(2.0f);
		}
		offset = offsetArg.getValue();
		format = formatArg.getValue();
		files  = filesArg.getValue();

	}
	catch(ArgException &e) {
		cerr << "Error: " << e.error() << " for arg " << e.argId() << endl;
		exit(1);
	}
}


// The entry point of the program
int main(int argc, char* argv[])
{
	QCoreApplication qtApp(argc, argv);

	tick_count t0 = tick_count::now();

	// Working parameters
	int offset;
	float exposure;
	bool srgb;
	float gamma;
	string format;
	vector<string> files;

	// Parses the arguments, the process will setup the exposure exponent, gamma and the list of files
        parseArgs(argc, argv, exposure, srgb, gamma, offset, format, files);

	// Creates the batch tone mapper with those arguments
	BatchToneMapper batchToneMapper(files);
	if (srgb) {
		batchToneMapper.setupToneMapper(exposure);
	}
	else {
		batchToneMapper.setupToneMapper(exposure, gamma);
	}
	batchToneMapper.setOffset(offset);
	batchToneMapper.setFormat(format);

	if( !batchToneMapper.hasWork() ) {
		cerr << "Error: there are no valid files to process." << endl;
		exit(1);
	}

	// Does all the magic!
	cout << batchToneMapper;
	batchToneMapper.execute();	

	tick_count tf = tick_count::now();
	cout << endl << "Processing took in total " << (tf-t0).seconds() << " seconds." << endl;

	return 0;
}

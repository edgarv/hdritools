// Main entry point for the console application
//
// Batch tonemapper. It reads the HDR files and all the images within the given zip files and
// tonemaps them using the specified parameters (gamma, exposure compensation and format).
// When the input filename contains a trailing number (i.e. file-0001.exr, 00003.rgbe) 
// an offset may be applied to generate the output file (i.e. with an offset=10; 
// file-0001.exr -> file-0011.png, 00003.rgbe -> 00013.png).
// The HDR supported formats are RGBE (.rgbe, .hdr), OpenEXR (.exr) and PFM (.pfm)
//
// TODO:
//   - Check/implement support for zip files with directories.
//

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

#include <iostream>

#include <tclap/CmdLine.h>
#include <tbb/tick_count.h>

// Main working entity
#include "ToneMappingFilter.h"
#include "BatchToneMapper.h"

// To get the list of formats
#include "Util.h"

#include <QCoreApplication>
#include <QString>
#include <QStringList>

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



#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
namespace
{
inline float log2f(float x)
{
    return logf(x) * 1.44269504088896340736f /* 1/logf(2.0f) */;
}
} // namespace
#endif


// Basically we have one constraint: for the exposure multiplier and gamma we
// want them to be greater than zero
class ConstraintGreaterThan : public Constraint<float> {

    const float minVal;
    string desc;
    const string short_id;

public:
    ConstraintGreaterThan(float mininum) : 
    minVal(mininum), short_id(mininum == 0.0f ? "possitive float" : "greaterThan")
    {
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


// Constraint for setting a minimum and maximum (inclusive)
template <typename T>
class ConstraintRange : public Constraint<T>
{
    const T m_min;
    const T m_max;
    string desc;
    string short_id;

public:
    ConstraintRange(T minimum, T maximum) : m_min(minimum), m_max(maximum)
    {
        ostringstream srange;
        srange << '[' << m_min << ',' << m_max << ']';
        short_id = "float in " + srange.str();
        desc = "The value must be in the range " + srange.str() + ".";
    }

    virtual string description() const {
        return desc;
    }
    virtual string shortID() const {
        return short_id;
    }
    virtual bool check(const T &value) const {
        return (m_min <= value) && (value <= m_max);
    }
};


// The format list is large, so we customize the builtin TCLAP constraint
class FormatConstraint : public ValuesConstraint<string>
{
public:
    FormatConstraint(vector<string> &allowed) :
    ValuesConstraint<string>(allowed), m_shortID("format_id")
    {
    }

    virtual string shortID() const {
        return m_shortID;
    }
    
private:
    const string m_shortID;
};



// Main parameter processing through TCLAP
void parseArgs(float &exposure, bool &srgb, float &gamma, bool &bpp16,
               pcg::TmoTechnique &technique,
               float &key, float &whitePoint, float &logLumAvg,
               int &offset, QString &format, QStringList &files) 
{
    try {

        CmdLine cmdline("Tone maps all the HDR images from the input files "
            "and those within the given zip files.", ' ',
            BatchToneMapper::getVersion().toStdString());
        ConstraintGreaterThan constraint(0.0f);
        ConstraintRange<float> constraintKey(0.0f, 1.0f);

        // Exposure
        ValueArg<float> exposureArg("e", "exposure", 
            "Exponent for the exposure compensation. "
            "The pixels will be multiplied by 2^exposure prior to "
            "gamma correction (default 0).", 
            false, 0.0f, "float");

        // Exposure factor
        ValueArg<float> exposureMultiplierArg("m", "expmult",
            "Exposure compensation multiplier. "
            "The pixels will be multiplied by this value prior to "
            "gamma correction (default 1).",
            false, 1.0f, &constraint);

        // Reinhard02 flag
        SwitchArg reinhard02Arg("",
            "reinhard02",
            "The pixels will be transformed by a curve defined by the global "
            "version of the Reinhard02 TMO. By default the tone mapping "
            "parameters are computed automatically for each image, but they "
            "may be overriden manually through the optional arguments "
            "--key, --whitepoint and --loglumavg. When all the optional "
            "arguments are set the tone mapping curve applied to each image "
            "becomes the same.",
            false);

        // Exposure, gamma and Reinhard02 are mutually exclusive
        vector<Arg*> tmoArgs;
        tmoArgs.push_back(&exposureArg);
        tmoArgs.push_back(&exposureMultiplierArg);
        tmoArgs.push_back(&reinhard02Arg);

        // Key (valid only with --reinhard02)
        ValueArg<float> keyArg("k", "key",
            "Tone mapping key. "
            "Overrides the value automatically set. "
            "Valid only when --reinhard02 is enabled.",
            false, ToneMappingFilter::AutoParam(), &constraintKey);

        // White point (valid only with --reinhard02)
        ValueArg<float> whitePointArg("w", "whitepoint",
            "Tone mapping white point. "
            "Overrides the value automatically set. "
            "Valid only when --reinhard02 is enabled.",
            false, ToneMappingFilter::AutoParam(), &constraint);

        // Average log luminance (valid only with --reinhard02)
        ValueArg<float> logLumAvgArg("l", "loglumavg",
            "Log-luminance average. "
            "Overrides the value automatically set. "
            "Valid only when --reinhard02 is enabled.",
            false, ToneMappingFilter::AutoParam(), &constraint);


        // Gamma value
        ValueArg<float> gammaArg("g", "gamma",
            "Gamma correction. "
            "The pixels will be raised to the power of 1/gamma after the "
            "low dynamic range conversion (default 2.2).",
            false, 2.2f, &constraint);

        // sRGB flag
        SwitchArg srgbArg("",
            "srgb",
            "The pixels will be transformed according to the sRGB space "
            "after the low dynamic range conversion.",
            false);

        // Offset to add to the numeric filenames
        ValueArg<int> offsetArg("o", "offset",
            "Filename offset. "
            "If the filename contains a trailing number, this value will be "
            "added to it for each final output file.",
            false, 0, "integer");

        // Output format constraint
        const QStringList & supportedFormatsQt = Util::supportedWriteImageFormats();
        vector<string> supportedFormats;
        for (QStringList::const_iterator it = supportedFormatsQt.constBegin();
             it != supportedFormatsQt.constEnd(); ++it)
        {
            supportedFormats.push_back(it->toStdString());
        }
        FormatConstraint formatConstraint(supportedFormats);
        const string defaultFormat = BatchToneMapper::getDefaultFormat().toStdString();

        string formatArgDesc("Image format for the tone mapped images. It must be one of: ");
        formatArgDesc += formatConstraint.description() + ". The default is " + defaultFormat + ".";
        ValueArg<string> formatArg("f", "format",
            formatArgDesc, 
            false, defaultFormat, &formatConstraint);

        // The unlabeled multiple arguments are the input zipfiles
        UnlabeledMultiArg<string> filesArg("filenames", 
            "HDR images (rgbe|hdr|exr|pfm) and Zip files with HDR images to tone map.", 
            true, "filename");

        // Adds the arguments to the command line
        // (the unlabeled multi args must be the last ones!!)
        cmdline.xorAdd(tmoArgs);
        cmdline.add(logLumAvgArg);
        cmdline.add(whitePointArg);
        cmdline.add(keyArg);
        cmdline.xorAdd(srgbArg, gammaArg);
        cmdline.add(offsetArg);
        cmdline.add(formatArg);
        cmdline.add(filesArg);
        
        // Parse the argv array, encoding the cmdline in UTF-8 for TCLAP
        const QStringList arguments = QCoreApplication::arguments();
        std::vector<std::string> args;
        for (QStringList::const_iterator it = arguments.constBegin();
             it != arguments.constEnd(); ++it) {
            const QByteArray utf8 = it->toUtf8();
            std::string arg(utf8.constData());
            args.push_back(arg);
        }

        cmdline.parse(args);


        // If we are here it's safe to recover the values
        srgb = srgbArg.getValue();
        gamma = gammaArg.getValue();
        exposure = exposureArg.getValue();
        if (exposureMultiplierArg.isSet()) {
            // Yes, is not that accurate but for our purposes it's enough
            exposure = log2f(exposureMultiplierArg.getValue());
        }

        technique = reinhard02Arg.getValue() ? pcg::REINHARD02 : pcg::EXPOSURE;
        key = keyArg.getValue();
        whitePoint = whitePointArg.getValue();
        logLumAvg = logLumAvgArg.getValue();

        offset = offsetArg.getValue();
        format = QString::fromStdString(formatArg.getValue());
        bpp16  = format == Util::PNG16_FORMAT_STR;
        const vector<string> &filesUtf8 = filesArg.getValue();
        for (vector<string>::const_iterator it = filesUtf8.begin();
             it != filesUtf8.end(); ++it) {
            files.append(QString::fromUtf8(it->c_str()));
        }
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
    bool bpp16;
    float gamma;
    pcg::TmoTechnique technique;
    float key, whitePoint, logLumAvg;
    QString format;
    QStringList files;

    // Parses the arguments
    parseArgs(exposure, srgb, gamma, bpp16, technique,
        key, whitePoint, logLumAvg, offset, format, files);

    // Creates the batch tone mapper with those arguments
    BatchToneMapper batchToneMapper(files, bpp16);
    if (srgb) {
        batchToneMapper.setupToneMapper(exposure);
    }
    else {
        batchToneMapper.setupToneMapper(exposure, gamma);
    }
    batchToneMapper.setTechnique(technique);
    if (technique == pcg::REINHARD02) {
        batchToneMapper.setReinhard02Params(key, whitePoint, logLumAvg);
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

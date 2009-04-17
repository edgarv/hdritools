#if !defined(TONEMAPPINGFILTER_H)
#define TONEMAPPINGFILTER_H

#include <ToneMapper.h>

// TBB import for the filter stuff
#include <tbb/pipeline.h>

using tbb::filter;
using pcg::ToneMapper;

// The class in charge of tone mapping. This guy is pretty transparent :)
class ToneMappingFilter : public tbb::filter {

private:

	const ToneMapper &toneMapper;
	const bool useBpp16;

public:
	// By default the tone mapped images are RGB with 8 bpp. If the
	// useBpp16 is activated, the saved images will have instead 16 bpp.
	// Note that this makes the tone mapping much slower, and so
	// far the only supported format is PNG.
	ToneMappingFilter(const ToneMapper &toneMapper, bool useBpp16);

	// This method receives pointers to ImageInfo structures.
	void* operator()(void* item);
};

#endif /* TONEMAPPINGFILTER_H */

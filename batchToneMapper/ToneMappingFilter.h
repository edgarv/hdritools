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

public:
	ToneMappingFilter(const ToneMapper &toneMapper);

	// This method receives pointers to ImageInfo structures.
	void* operator()(void* item);
};

#endif /* TONEMAPPINGFILTER_H */

// A place for the static variables
#include "Rgba32F.h"

using namespace pcg;

const ALIGN16_BEG unsigned int Rgba32F::alpha_zero_mask[4] ALIGN16_END = 
    {0x0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
const ALIGN16_BEG unsigned int Rgba32F::abs_mask[4] ALIGN16_END = 
    {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};


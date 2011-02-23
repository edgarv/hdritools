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

// A place for the static variables
#include "Rgba32F.h"

using namespace pcg;

const ALIGN16_BEG unsigned int Rgba32F::alpha_zero_mask[4] ALIGN16_END = 
    {0x0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
const ALIGN16_BEG unsigned int Rgba32F::abs_mask[4] ALIGN16_END = 
    {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};


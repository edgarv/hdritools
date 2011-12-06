/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

// Common utilities

#if defined(_MSC_VER)
# pragma once
#endif

#if !defined(PCH_UTIL_H)
#define PCH_UTIL_H

namespace pcg
{

// Get the number of CPUs in the system
int getNumCPUs();

// Generic initialization for Matlab: sets up the number of threads to
// use in OpenEXR and registers the exit callback.
void mexEXRInit();

} // namespace pcg

#endif

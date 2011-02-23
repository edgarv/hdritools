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

#if defined(_MSC_VER)
#pragma once
#endif
#if !defined(PCG_IMAGEIO_VERSION_H)
#define PCG_IMAGEIO_VERSION_H

#cmakedefine01 HDRITOOLS_HAS_VALID_REV

namespace pcg
{
    namespace version
    {
        const int MAJOR = ${HDRITOOLS_VERSION_MAJOR};
        const int MINOR = ${HDRITOOLS_VERSION_MINOR};
        const int PATCH = ${HDRITOOLS_VERSION_PATCH};
        const int BUILD = ${HDRITOOLS_VERSION_BUILD};

        inline const char* versionString()
        {
            return "${HDRITOOLS_VERSION_MAJOR}.${HDRITOOLS_VERSION_MINOR}.${HDRITOOLS_VERSION_PATCH}";
        }
        
        inline const char* buildDateString()
        {
            return "${HDRITOOLS_DATE}";
        }

        inline const char* globalRevision()
        {
            return "${HDRITOOLS_REV_ID}";
        }
    } // namespace version
} // namespace pcg

#endif // PCG_IMAGEIO_VERSION_H

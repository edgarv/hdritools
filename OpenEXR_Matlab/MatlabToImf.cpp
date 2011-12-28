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

#include "MatlabToImf.h"

#include <mex.h>

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wlong-long"
#endif

#include <ImfAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfDoubleAttribute.h>

#ifdef __clang__
# pragma clang diagnostic pop
#endif

#include <string>


using namespace Imf;


namespace
{

template <typename T>
T* toAttributeImp(const mxArray * pa)
{
    mexWarnMsgIdAndTxt("OpenEXR:unsupported",
        "Unsupported conversion to Imf::Attribute from \"%s\"", mxGetClassName(pa));
    return NULL;
}


template <>
StringAttribute* toAttributeImp(const mxArray * pa)
{
    std::string value;
    pcg::toNativeCheck(pa, value);
    StringAttribute * attr = new StringAttribute(value);
    return attr;
}


template <>
StringVectorAttribute* toAttributeImp(const mxArray * pa)
{
    StringVector value;
    const mwSize numel = mxGetNumberOfElements(pa);
    for (mwIndex i = 0; i != numel; ++i) {
        char * txt = mxArrayToString(mxGetCell(pa, i));
        assert(txt != NULL);
        value.push_back(std::string(txt));
        mxFree(txt);
    }
    StringVectorAttribute * attr = new StringVectorAttribute(value);
    return attr;
}


template <>
IntAttribute* toAttributeImp(const mxArray * pa)
{
    int value = 0;
    pcg::convertData(&value, pa, mxGetClassID(pa), 1);
    IntAttribute* attr = new IntAttribute(value);
    return attr;
}


template <>
FloatAttribute* toAttributeImp(const mxArray * pa)
{
    float value = 0.0f;
    pcg::convertData(&value, pa, mxGetClassID(pa), 1);
    FloatAttribute* attr = new FloatAttribute(value);
    return attr;
}


template <>
DoubleAttribute* toAttributeImp(const mxArray * pa)
{
    double value = 0.0;
    pcg::convertData(&value, pa, mxGetClassID(pa), 1);
    DoubleAttribute* attr = new DoubleAttribute(value);
    return attr;
}


}


Attribute* pcg::toAttribute(const mxArray * pa)
{
    if (mxIsChar(pa)) {
        return toAttributeImp<StringAttribute> (pa);
    }
    else if (mxIsCell(pa)) {
        mwSize numel = 0;
        if (isVector(pa, numel)) {
            // First pass: check if all are strings
            for (mwIndex i = 0; i != numel; ++i) {
                if (!mxIsChar(mxGetCell(pa, i))) {
                    return NULL;
                }
            }
            return toAttributeImp<StringVectorAttribute> (pa);
        }
    }
    else if (mxIsNumeric(pa)) {
        mwSize M = 0, N = 0;
        if (!isMatrix(pa, M, N)) {
            return NULL;
        }

        if (M == 1 && N == 1) {
            const mxClassID type = mxGetClassID(pa);
            switch(type) {
            case mxSINGLE_CLASS:
                return toAttributeImp<FloatAttribute> (pa);
            case mxINT8_CLASS:
            case mxUINT8_CLASS:
            case mxINT16_CLASS:
            case mxUINT16_CLASS:
            case mxINT32_CLASS:
                return toAttributeImp<IntAttribute> (pa);
            case mxUINT32_CLASS:
            case mxINT64_CLASS:
            case mxUINT64_CLASS:
            case mxDOUBLE_CLASS:
                return toAttributeImp<DoubleAttribute> (pa);
            default:
                mexWarnMsgIdAndTxt("OpenEXR:unsupported",
                    "Unknown scalar type: \"%s\"", mxGetClassName(pa));
                return NULL;
            }
        }
    }

    return NULL;
}

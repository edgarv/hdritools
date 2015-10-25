///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002-2012, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////



//---------------------------------------------------------------------
//
//	Constructors and destructors for our exception base class.
//
//---------------------------------------------------------------------

#include "IexExport.h"
#include "IexBaseExc.h"
#include "IexMacros.h"

#include <stdlib.h>

IEX_INTERNAL_NAMESPACE_SOURCE_ENTER


namespace {

StackTracer currentStackTracer = 0;

} // namespace


void	
setStackTracer (StackTracer stackTracer)
{
    currentStackTracer = stackTracer;
}


StackTracer
stackTracer ()
{
    return currentStackTracer;
}


struct BaseExc::Data
{
    Data(const char* s) throw() :
        text(s? s: ""),
        stackTrace (currentStackTracer? currentStackTracer(): "")
    {
        // empty
    }
    Data(const std::string& s) throw() :
        text(s),
        stackTrace (currentStackTracer? currentStackTracer(): "")
    {
        // empty
    }
    explicit Data(const Data* d) throw() :
        text(d->text),
        stackTrace(d->stackTrace)
    {
        // empty
    }

    std::string text;
    std::string stackTrace;
};


BaseExc::BaseExc (const char* s) throw () :
    _d (new Data (s? s: ""))
{
    // empty
}


BaseExc::BaseExc (const std::string &s) throw () :
    _d (new Data (s))
{
    // empty
}


BaseExc::BaseExc (std::stringstream &s) throw () :
    _d (new Data (s.str()))
{
    // empty
}


BaseExc::BaseExc (const BaseExc &be) throw () :
    _d (new Data (be._d))
{
    // empty
}


BaseExc::~BaseExc () throw ()
{
    delete _d;
}


const char *
BaseExc::what () const throw ()
{
    return _d->text.c_str();
}


BaseExc &
BaseExc::assign (const std::string &s)
{
    _d->text.assign (s);
    return *this;
}

BaseExc &
BaseExc::append (const std::string &s)
{
    _d->text.append (s);
    return *this;
}


BaseExc &
BaseExc::assign (std::stringstream &s)
{
    _d->text.assign (s.str());
    return *this;
}

BaseExc &
BaseExc::append (std::stringstream &s)
{
    _d->text.append (s.str());
    return *this;
}


BaseExc &
BaseExc::assign (const char *s)
{
    _d->text.assign (s);
    return *this;
}

BaseExc &
BaseExc::append (const char *s)
{
    _d->text.append (s);
    return *this;
}


bool
BaseExc::operator== (const std::string &s) const
{
    return _d->text == s;
}

bool
BaseExc::operator!= (const std::string &s) const
{
    return _d->text != s;
}


const std::string &
BaseExc::stackTrace () const
{
    return _d->stackTrace;
}


std::ostream &
operator<< (std::ostream &os, const BaseExc &be)
{
    return os << be._d->text;
}

IEX_INTERNAL_NAMESPACE_SOURCE_EXIT


#if _MSC_VER >= 1400

#include <intrin.h>
#pragma optimize("", off)
void
iex_debugTrap()
{
    size_t requiredSize;
    getenv_s(&requiredSize, NULL, 0, "IEXDEBUGTHROW");
    if (requiredSize != 0)
        __debugbreak();
}
#elif defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma optimize("", off)
void
iex_debugTrap()
{
    if (0 != getenv("IEXDEBUGTHROW"))
        ::DebugBreak();
}
#else
void
iex_debugTrap()
{
    // how to in Linux?
    if (0 != ::getenv("IEXDEBUGTHROW"))
        __builtin_trap();
}
#endif

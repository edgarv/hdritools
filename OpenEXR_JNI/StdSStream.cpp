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

///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Industrial Light & Magic nor the names of
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
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.67
//
///////////////////////////////////////////////////////////////////////////

#include "StdSStream.h"

#include <Iex.h>
#include <cerrno>


namespace
{

inline void clearError() {
    errno = 0;
}


inline bool checkError(std::istringstream& is, std::streamsize expected = 0) {
    if (!is) {
        if (errno) {
            Iex::throwErrnoExc();
        } else if (is.gcount() < expected) {
            THROW (Iex::InputExc, "Early end of file: read " << is.gcount()
                << " out of " << expected << " requested bytes.");
        }
        return false;
    }
    return true;
}


inline void checkError (std::ostringstream &os) {
    if (!os) {
        if (errno) {
            Iex::throwErrnoExc();
        } else {
            throw Iex::ErrnoExc ("File output failed.");
        }
    }
}

} // namespace



StdISStream::StdISStream(const std::string& str) :
Imf::IStream("(string)"), m_is(str)
{
    // empty
}

bool StdISStream::read(char c[/*n*/], int n)
{
    if (!m_is) {
        throw Iex::InputExc("Unexpected end of stream.");
    }
    clearError();
    m_is.read(c, n);
    return checkError(m_is, n);
}

Imath::Int64 StdISStream::tellg()
{
    return std::streamoff(m_is.tellg());
}

void StdISStream::seekg(Imath::Int64 pos)
{
    m_is.seekg(pos);
    checkError(m_is);
}

void StdISStream::clear()
{
    m_is.clear();
}



StdOSStream::StdOSStream() : Imf::OStream ("(string)")
{
    // empty
}

void StdOSStream::write(const char c[/*n*/], int n)
{
    clearError();
    m_os.write(c, n);
    checkError(m_os);
}

Imath::Int64 StdOSStream::tellp()
{
    return std::streamoff(m_os.tellp());
}

void StdOSStream::seekp(Imath::Int64 pos)
{
    m_os.seekp(pos);
    checkError(m_os);
}

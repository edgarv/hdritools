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
// Copyright (c) 2005, Industrial Light & Magic, a division of Lucas
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

package edu.cornell.graphics.exr;

/**
 * Threading support for the IlmImf library.
 * 
 * <p>The IlmImf library uses threads to perform reading and writing
 * of OpenEXR files in parallel.  The thread that calls the library
 * always performs the actual file IO (this is usually the main
 * application thread) whereas several worker threads perform
 * data compression and decompression.  The number of worker
 * threads can be any non-negative value (a value of zero reverts
 * to single-threaded operation).  As long as there is at least
 * one worker thread, file IO and compression can potentially be
 * done concurrently through pinelining.  If there are two or more
 * worker threads, then pipelining as well as concurrent compression
 * of multiple blocks can be performed.</p>
 * 
 * <p>Threading in the Imf library is controllable at two granularities:
 * <ul>
 * <li>The functions in this file query and control the total number
 *     of worker threads, which will be created globally for the whole
 *     library.  Regardless of how many input or output files are
 *     opened simultaneously, the library will use at most this number
 *     of worker threads to perform all work.  The default number of
 *     global worker threads is zero (i.e. single-threaded operation;
 *     everything happens in the thread that calls the library.)</li>
 * 
 * <li>Furthermore, it is possible to set the number of threads that
 *     each input or output file should keep busy.  This number can
 *     be explicitly set for each file.  The default behavior is for
 *     each file to try to occupy all worker threads in the library's
 *     thread pool.</li>
 * </ul></p>
 */
public final class Threading {
    
    static {
        // TODO Unify native library loading
        System.loadLibrary("openexrjni");
    }
    
    /**
     * Return the number of global worker threads used for parallel
     * compression and decompression of OpenEXR files.
     */
    public static synchronized native int globalThreadCount();
    
    /**
     * Change the number of global worker threads to {@code count}, a value
     * greater than or equal to {@literal 0}.
     * @param count the new number of global worker threads
     */
    public static synchronized native void setGlobalThreadCount(int count);
    
}

/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

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
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

package edu.cornell.graphics.exr;

import edu.cornell.graphics.exr.attributes.EnvMapAttribute;

/**
 * Types of environment maps.
 * 
 * <p>Environment maps define a mapping from 3D directions to 2D
 * pixel space locations. Environment maps are typically used
 * in 3D rendering, for effects such as quickly approximating
 * how shiny surfaces reflect their environment.</p>
 * 
 * <p>Environment maps can be stored in scanline-based or in tiled
 * OpenEXR files. The fact that an image is an environment map
 * is indicated by the presence of an {@link EnvMapAttribute} whose name
 * is "envmap". (Convenience functions to access this attribute
 * are defined in @{@link StandardAttributes}.)
 * The attribute's value defines the mapping from 3D directions
 * to 2D pixel space locations.</p>
 * 
 * @since OpenEXR-JNI 2.1
 */
public enum EnvMap {
    /**
     * Latitude-longitude environment map.
     * 
     * <p>The environment is projected onto the image using polar coordinates
     * (latitude and longitude). A pixel's x coordinate corresponds to
     * its longitude, and the y coordinate corresponds to its latitude.
     * Pixel {@code (dataWindow.min.x, dataWindow.min.y)} has latitude
     * {@code +pi/2} and longitude {@code +pi}; 
     * pixel {@code (dataWindow.max.x, dataWindow.max.y)} has
     * latitude {@code -pi/2} and longitude {@code -pi}.</p>
     * 
     * <p>In 3D space, latitudes {@code -pi/2} and {@code +pi/2} correspond to
     * the negative and positive y direction. Latitude 0, longitude 0 points 
     * into positive z direction; and latitude 0, longitude {@code pi/2} points
     * into positive x direction.</p>
     * 
     * <p>The size of the data window should be {@code 2*N} by {@code N} pixels
     * (width by height), where {@code N} can be any integer greater
     * than {@literal 0}.</p>
     */
    LATLONG, // 0
    
    /**
     * Cube map.
     * 
     * <p>The environment is projected onto the six faces of an
     * axis-aligned cube. The cube's faces are then arranged
     * in a 2D image as shown below.
     * <blockquote><pre>
     *           2-----------3
     *          /           /|
     *         /           / |       Y
     *        /           /  |       |
     *       6-----------7   |       |
     *       |           |   |       |
     *       |           |   |       |
     *       |   0       |   1       *------- X
     *       |           |  /       /
     *       |           | /       /
     *       |           |/       /
     *       4-----------5       Z
     * 
     *    dataWindow.min
     *         /
     *        / 
     *       +-----------+
     *       |3    Y    7|
     *       |     |     |
     *       |     |     |
     *       |  ---+---Z |  +X face
     *       |     |     |
     *       |     |     |
     *       |1         5|
     *       +-----------+
     *       |6    Y    2|
     *       |     |     |
     *       |     |     |
     *       | Z---+---  |  -X face
     *       |     |     |
     *       |     |     |
     *       |4         0|
     *       +-----------+
     *       |6    Z    7|
     *       |     |     |
     *       |     |     |
     *       |  ---+---X |  +Y face
     *       |     |     |
     *       |     |     |
     *       |2         3|
     *       +-----------+
     *       |0         1|
     *       |     |     |
     *       |     |     |
     *       |  ---+---X |  -Y face
     *       |     |     |
     *       |     |     |
     *       |4    Z    5|
     *       +-----------+
     *       |7    Y    6|
     *       |     |     |
     *       |     |     |
     *       | X---+---  |  +Z face
     *       |     |     |
     *       |     |     |
     *       |5         4|
     *       +-----------+
     *       |2    Y    3|
     *       |     |     |
     *       |     |     |
     *       |  ---+---X |  -Z face
     *       |     |     |
     *       |     |     |
     *       |0         1|
     *       +-----------+
     *                  /
     *                 /
     *           dataWindow.max</pre></blockquote>
     * 
     * <p>The size of the data window should be {@code N} by {@code 6*N} pixels
     * (width by height), where {@code N} can be any integer greater
     * than {@literal 0}.</p>
     */
    CUBE // 1
    
}

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

package edu.cornell.graphics.exr;

/**
 * Layout of the pixel data in an OpenEXR image file.
 * 
 * @since OpenEXR-JNI 2.1
 */
public enum LineOrder {
    /**
     * First scan line has lowest y coordinate. */
    INCREASING_Y, // 0
    
    /** First scan line has highest y coordinate. */
    DECREASING_Y, // 1
    
    /** Only for tiled files; tiles are written in random order. */
    RANDOM_Y // 2
}

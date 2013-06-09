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
package edu.cornell.graphics.exr;

/**
 * Signals that an OpenEXR file does not contain the requested channel.
 * 
 * @since OpenEXR-JNI 2.0
 */
class NoSuchChannelException extends RuntimeException {
    private static final long serialVersionUID = -7878358869230043028L;

    public NoSuchChannelException(Throwable cause) {
        super(cause);
    }

    public NoSuchChannelException(String message, Throwable cause) {
        super(message, cause);
    }

    public NoSuchChannelException(String message) {
        super(message);
    }

    public NoSuchChannelException() {
    }
}

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

import java.io.IOException;

/**
 * Signals that an I/O exception or file format error of some sort has occurred
 * while reading or writing OpenEXR files.
 *
 * @since OpenEXR-JNI 2.0
 */
public class EXRIOException extends IOException {
    private static final long serialVersionUID = 6976890346603381465L;

    /** 
     * Constructs a new {@code EXRIOException} with {@code null} as its
     * detail message.  The cause is not initialized, and may subsequently be
     * initialized by a call to {@link #initCause}.
     */
    public EXRIOException() {
        super();
    }
    
    /**
     * Constructs a {@code EXRIOException} with the specified
     * detail message. The cause is not initialized, and may subsequently be
     * initialized by a call to {@link #initCause}.
     *
     * @param   message   the detail message. The detail message is saved for
     *          later retrieval by the {@link #getMessage()} method.
     */
    public EXRIOException(String message) {
        super(message);
    }

    /** 
     * Constructs a new {@code EXRIOException} with the specified cause and a
     * detail message of {@code (cause==null ? null : cause.toString())}
     * (which typically contains the class and detail message of
     * {@code cause}.)  This constructor is useful if the
     * {@code EXRIOException} being thrown is little more than a wrapper for
     * other throwable.
     *
     * @param  cause the cause (which is saved for later retrieval by the
     *         {@link #getCause()} method.)  (A {@code null} value is
     *         permitted, and indicates that the cause is nonexistent or
     *         unknown.)
     */
    public EXRIOException(Throwable cause) {
        super(cause);
    }

    /**
     * Constructs a new {@code EXRIOException} with the specified 
     * detail message and cause.
     * 
     * <p>Note that the detail message associated with
     * {@code cause} is <em>not</em> automatically incorporated in
     * this exception's detail message.</p>
     *
     * @param  message the detail message (which is saved for later retrieval
     *         by the {@link #getMessage()} method.)
     * @param  cause the cause (which is saved for later retrieval by the
     *         {@link #getCause()} method.)  (A {@code null} value is
     *         permitted, and indicates that the cause is nonexistent or
     *         unknown.)
     */
    public EXRIOException(String message, Throwable cause) {
        super(message, cause);
    }
    
}

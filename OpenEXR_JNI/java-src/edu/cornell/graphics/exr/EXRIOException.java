package edu.cornell.graphics.exr;

import java.io.IOException;

/**
 * This class represents the exceptions generated by error while using
 * the native OpenEXR library.
 * 
 * @author edgar
 *
 */
public class EXRIOException extends IOException {

	private static final long serialVersionUID = 1L;

	public EXRIOException(){}
	public EXRIOException(String message) {
		super(message);
	}
}

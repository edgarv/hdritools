/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2013 Program of Computer Graphics, Cornell University

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
 * Package-internal class to consolidate loading the native library.
 * 
 * @since OpenEXR-JNI 3.0
 */
final class NativeLibraryLoader {
    
    /** Default name of the native library */
    private final static String JNI_LIBNAME = "openexrjni3";
    
    /** Flag to know if the library has already been loaded */
    private static volatile boolean loaded = false;
    
    private NativeLibraryLoader() {
        // empty
    }
    
    /**
     * Loads the OpenEXR-JNI native library. If it has already been loaded,
     * the second and subsequent calls are ignored.
     * 
     * @throws UnsatisfiedLinkError if the native library cannot be found
     * @see System#loadLibrary(String)
     */
    static void loadLibrary() {
        // Use the double locking pattern for better performance
        if (!loaded) {
            synchronized (NativeLibraryLoader.class) {
                if (!loaded) {
                    System.loadLibrary(JNI_LIBNAME);
                    loaded = true;
                }
            }
        }
    }
    
}

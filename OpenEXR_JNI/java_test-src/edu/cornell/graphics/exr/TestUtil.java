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

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.file.FileSystem;
import java.nio.file.FileSystemNotFoundException;
import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;

/**
 * Helper method for the test cases.
 */
public final class TestUtil {
    
    private TestUtil() {
        // empty
    }
    
    /**
     * Returns a {@code Path} to a system resource. The resource's URL is
     * resolved by calling {@link ClassLoader#getResource(java.lang.String)}.
     * 
     * <p>This method supports resources contained in a jar file. In that case
     * the method tries to create a new ZIP file system for the file (which
     * will remain open until the JVM finishes its execution.)
     * 
     * @param resource the resource name
     * @return a {@code Path} to the system resource
     * @throws IOException if the resource's path cannot be obtained or some
     *         other I/O error occurs
     */
    public static Path getResourcePath(String resource) throws IOException {
        URL url = ClassLoader.getSystemResource(resource);
        if (url == null) {
            throw new IOException("Could not open resource: " + resource);
        }
        URI uri;
        try {
            uri = url.toURI();
        } catch (URISyntaxException ex) {
            throw new IllegalStateException(ex.getMessage(), ex);
        }
        Path path;
        if (uri.getScheme().equalsIgnoreCase("jar")) {
            FileSystem fs;
            try {
                fs = FileSystems.getFileSystem(uri);
            } catch (FileSystemNotFoundException ex) {
                HashMap<String, String> env = new HashMap<>();
                env.put("create", "false");
                fs = FileSystems.newFileSystem(uri, env);
            }
            path = fs.getPath(resource);
        } else {
            path = Paths.get(uri);
        }
        return path;
    }
    
}

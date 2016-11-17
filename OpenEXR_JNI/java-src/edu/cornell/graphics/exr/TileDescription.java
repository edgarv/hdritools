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
 * For tiled files, specifies the size of the tiles, and the file's level mode.
 * 
 * <p>A single tiled OpenEXR files may contain multiple versions of the 
 * same image, each with a different resolution. Each version is called 
 * a <em>level</em>. The number of levels in a file and their resolutions
 * depend on the file's level mode.</p>
 * 
 * <p>Levels are identified by <em>level numbers</em>. A level number is a pair
 * of integers, <tt>(l_x,l_y)</tt>. Level {@code (0,0)} is
 * the highest-resolution level, with {@code w} by {@code h} pixels.
 * Level <tt>(l_x,l_y)</tt> contains
 * <blockquote><tt>rf(w / pow(2,l_x))</tt></blockquote>
 * by
 * <blockquote><tt>rf(w / pow(2,l_y))</tt></blockquote>
 * pixels, where {@code rf(x)} is a rounding function, either {@code floor(x)}
 * or {@code ceil(x)}, depending on the file's
 * <em>level size rounding mode</em> ({@code ROUND_DOWN} or {@code ROUND_UP}.)
 * 
 * @since OpenEXR-JNI 2.1
 */
public class TileDescription implements Cloneable {
    
    /**
     * Describes how many versions of the same image a file contains as well
     * as their resolutions.
     */
    public static enum LevelMode {
        /**
         * The file contains only a single full-resolution level. <p>A tiled
         * {@code ONE_LEVEL} file is equivalent to a scan-line-based file; 
         * the only difference is that pixels are accessed by tile rather 
         * than by scan line.</p>
         */
        ONE_LEVEL,
        
        /**
         * The file contains multiple versions of the image.
         * 
         * <p>Each successive level is half the resolution of the previous 
         * level in both dimensions. The lowest-resolution level contains only 
         * a single pixel. For example, if the first level, with full 
         * resolution, contains 16&times;8 pixels, then the file contains four more 
         * levels with 8&times;4, 4&times;2, 2&times;1, and 1&times;1 pixels respectively.
         */
        MIPMAP_LEVELS,
        
        /**
         * Like {@link #MIPMAP_LEVELS}, but with more levels. 
         * 
         * <p>The levels include all combinations of reducing the resolution 
         * of the first level by powers of two independently in both dimensions.
         * For example, if the first level contains 4&times;4 pixels, then the file
         * contains eight more levels, with the following resolutions:
         * <blockquote><pre>
         *      2x4  1x4
         * 4x2  2x2  1x2
         * 4x1  2x1  1x1
         * </pre>
         * </blockquote>
         */
        RIPMAP_LEVELS
    }
    
    /**
     * Specifies the level size rounding function. The rounding mode is
     * used to compute the number of levels in a file.
     */
    public static enum RoundingMode {
        /** Use {@code floor(x)} as the level size rounding function. */
        ROUND_DOWN,
        /** Use {@code ceil(x)} as the level size rounding function. */
        ROUND_UP
    }
    
    /** Width of each tile in pixels. */
    public int xSize = 32;
    /** Height of each tile in pixels. */
    public int ySize = 32;
    /** Image file's level mode. */
    public LevelMode mode = LevelMode.ONE_LEVEL;
    /** Image file's level size rounding mode. */
    public RoundingMode roundingMode = RoundingMode.ROUND_DOWN;

    /**
     * Compares this tile description to the specified object. The result is
     * {@code true} if and only if the argument is not {@code null}, is a
     * {@code TileDescription} object, and their tile dimensions, level mode
     * and rounding mode are all equal.
     *
     * @param obj The object to compare this {@code TileDescription} against
     * @return {@code true} if the given object represents a
     *         {@code TileDescription} equivalent to this instance,
     *         {@code false} otherwise
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final TileDescription other = (TileDescription) obj;
        if (this.xSize != other.xSize) {
            return false;
        }
        if (this.ySize != other.ySize) {
            return false;
        }
        if (this.mode != other.mode) {
            return false;
        }
        if (this.roundingMode != other.roundingMode) {
            return false;
        }
        return true;
    }

    /**
     * Returns a hash code for this tile description. The hash code for a 
     * {@code TileDescription<T>} object is computed as the aggregate of the 
     * tile size, level mode and level size rounding mode.
     * 
     * @return a hash code for this tile description
     */
    @Override
    public int hashCode() {
        int hash = 3;
        hash = 29 * hash + this.xSize;
        hash = 29 * hash + this.ySize;
        hash = 29 * hash + (this.mode != null ? this.mode.hashCode() : 0);
        hash = 29 * hash + (this.roundingMode != null ?
                this.roundingMode.hashCode() : 0);
        return hash;
    }

    /**
     * Creates and returns a copy of this tile description.
     * @return a clone of this tile description
     */
    @Override
    public TileDescription clone() {
        try {
            return (TileDescription) super.clone();
        } catch (CloneNotSupportedException ex) {
            throw new IllegalStateException("Clone failed", ex);
        }
    }
    
}

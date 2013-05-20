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
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
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

import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Set;
import java.util.TreeMap;

/**
 * List of channels present in a file. Each channel has a uniquely associated
 * non-empty name.
 */
public class ChannelList implements Iterable<ChannelList.ChannelListElement> {
    
    /** Helper public interface to iterate over channel data */
    public static interface ChannelListElement {
        /**
         * Logical name associated to this channel.
         * @return a String
         */
        String getName();
        
        /**
         * Data format description for the channel
         * @return the channel description
         */
        Channel getChannel();
    }

    /**
     * Returns an iterator to the channel elements, that is the tuple of
     * a channel name and its channel data description. Its behavior is
     * undefined if the underlying channel list is modified.
     * 
     * @return a read-only iterator
     */
    @Override
    public Iterator<ChannelListElement> iterator() {
        return new Iterator<ChannelListElement>() {
            
            private final Iterator<Entry<String,Channel>> it =
                    map.entrySet().iterator();
            private Entry<String,Channel> curr = null;
            
            private final ChannelListElement elem  = new ChannelListElement() {

                @Override
                public String getName() {
                    return curr.getKey();
                }

                @Override
                public Channel getChannel() {
                    return curr.getValue();
                }
            };

            @Override
            public boolean hasNext() {
                return it.hasNext();
            }

            @Override
            public ChannelListElement next() {
                curr = it.next();
                return elem;
            }

            /**
             * Unsupported operation, throws
             * {@code UnsupportedOperationException}
             */
            @Override
            public void remove() {
                throw new UnsupportedOperationException("Not supported.");
            }
        };
    }
    
    
    // Set of channels sorted by name
    private final TreeMap<String, Channel> map = new TreeMap<>();
    
    /**
     * Add a channel to the current channel list. The {@code name} cannot
     * be empty nor exceed the maximum name length for a given version of
     * the OpenEXR header; the channel description cannot be null.
     * Returns the previous channel description associated with that name,
     * or {@code null} if there was no mapping for the name.
     * 
     * @param name name for the channel
     * @param channel data description for the channel
     * @return the previous channel description associated with that name,
     *         or {@code null} if there was no mapping for the name.
     */
    public Channel insert(String name, Channel channel) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Invalid channel name.");
        } else if (channel == null) {
            throw new IllegalArgumentException("Invalid channel description.");
        }
        return map.put(name, channel);
    }
    
    /**
     * Return a reference to the description of an existing channel with
     * the given name. If such a channel does not exist it throws
     * {@code IllegalArgumentException}
     * 
     * @param name name associated with the channel
     * @return a reference to the channel description
     * @throws IllegalArgumentException if there is not such a channel
     * @see #findChannel(java.lang.String) 
     */
    public Channel getChannel(String name) throws IllegalArgumentException {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Invalid channel name.");
        }
        final Channel c = map.get(name);
        if (c != null) {
            return c;
        } else {
            throw new IllegalArgumentException("There is no channel " + name);
        }
    }
    
    /**
     * Return a reference to the description of an existing channel with
     * the given name. If such a channel does not exist it return {@code null}.
     * 
     * @param name name associated with the channel
     * @return a reference to the channel description or {@code null}.
     * @see #getChannel(java.lang.String) 
     */
    public Channel findChannel(String name) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Invalid channel name.");
        }
        return  map.get(name);
    }
    
    /**
     * Returns {@code true} if this channel list has a channel with the given
     * name. Thus if this method returns {@code true}, calling 
     * {@code getChannel} with the same channel name will not throw
     * throw an exception.
     * 
     * @param name channel name whose presence is to be tested.
     * @return {@code true} if this channel list has a channel with the given
     *         name
     */
    public boolean containsChannel(String name) {
        return map.containsKey(name);
    }
    
    /**
     * Remove a channel from the current channel list.
     * Returns the previous channel description associated with that name,
     * or {@code null} if there was no mapping for the name.
     * 
     * @param name name for the channel to remove
     * @return the previous channel description associated with that name,
     *         or {@code null} if there was no mapping for the name.
     */
    public Channel removeChannel(String name) {
        return map.remove(name);
    }
    
    /**
     * Removes all the channels from the list.
     */
    public void clear() {
        map.clear();
    }
    
    /**
     * Returns the number of channels in this list.
     * @return the number of channels in this list
     */
    public int size() {
        return map.size();
    }
    
    /**
     * Return {@code true} if this channel list does not contain any mappings.
     * @return {@code true} if this channel list does not contain any mappings
     */
    public boolean isEmpty() {
        return map.isEmpty();
    }
    
    /**
     * Return a read-only view of the channels names contained in this channel
     * list. The set's iterator returns the names in ascending order.
     * If the channel list is modified while an iteration over the set is in
     * progress, the results of the iteration are undefined.
     * @return a read-only view of the channels' names
     */
    public Set<String> nameSet() {
        return Collections.unmodifiableSet(map.keySet());
    }
    
    /**
     * Return a read-only view of the channel descriptions contained in this
     * channel list. The collection's iterator returns the channel in ascending
     * order according to their associated name. If the channel list is modified
     * while an iteration over the collection is in progress, the results of the
     * iteration are undefined.
     * @return a read-only view of the channels' descriptions
     */
    public Collection<Channel> channels() {
        return Collections.unmodifiableCollection(map.values());
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final ChannelList other = (ChannelList) obj;
        if (this.map != other.map &&
                (this.map == null || !this.map.equals(other.map))) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        int hash = 5;
        hash = 67 * hash + (this.map != null ? this.map.hashCode() : 0);
        return hash;
    }

    /**
     * Returns a string representation of this channel list. The string
     * representation consists of a list of name-channel mappings in ascending
     * order by channel name, enclosed in braces
     * (<tt>"{}"</tt>).  Adjacent channels are separated by the characters
     * <tt>", "</tt> (comma and space).  Each name-channel mapping is rendered
     * as the name followed by an equals sign (<tt>"="</tt>) followed by the
     * associated channel description.
     *
     * @return a string representation of this channel list
     */
    @Override
    public String toString() {
        return map.toString();
    }
        
}

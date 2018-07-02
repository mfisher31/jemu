/*
    Copyright 2007-2012 David Robillard <http://drobilla.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include "jemu/util.h"

namespace jemu {

class RingBuffer
{
public:
    /** Create a new ring buffer */

    explicit RingBuffer (uint32_t capacity)
        : bufferSize (jemu::nextPowerOfTwo (capacity)),
          sizeMask (bufferSize - 1),
          buffer (static_cast<char*> (std::malloc (bufferSize)))
    {
        reset();
        assert (readSpace() == 0);
        assert (writeSpace() == bufferSize - 1);
    }

    inline ~RingBuffer()
    {
        std::free (buffer);
    }

    /** Resize the RingBuffer.

        This method is NOT thread-safe, it may only be called when there are no
        readers or writers.
    */
    inline void resize (const uint32_t newSize)
    {
        const auto nextSize (jemu::nextPowerOfTwo (newSize));
        if (nextSize == bufferSize)
            return;
        bufferSize = nextSize;
        sizeMask = bufferSize - 1;
        std::free (buffer);
        buffer = static_cast<char*> (std::malloc (bufferSize));
        reset();
    }

    /** Reset (empty) the RingBuffer.

        This method is NOT thread-safe, it may only be called when there are no
        readers or writers.
      */
    inline void reset()
    {
        writeHead = 0;
        readHead  = 0;
    }

    /** Clear the RingBuffer. Alias of RingBuffer::reset */
    inline void clear() { reset(); }

    /** Return the number of bytes of space available for reading. */
    inline uint32_t readSpace() const
    {
        return readSpaceInternal (readHead, writeHead);
    }

    /** Return the number of bytes of space available for writing. */
    inline uint32_t writeSpace() const
    {
        return writeSpaceInternal (readHead, writeHead);
    }

    /** Return the capacity (i.e. total write space when empty). */
    inline uint32_t capacity() const { return bufferSize - 1; }

    /** Read from the RingBuffer without advancing the read head. */
    inline uint32_t peek (uint32_t size, void* dst)
    {
        return peekInternal (readHead, writeHead, size, dst);
    }

    /** Read from the RingBuffer and advance the read head. */
    inline uint32_t read (uint32_t size, void* dst)
    {
        const uint32_t r = readHead;
        const uint32_t w = writeHead;

        if (peekInternal (r, w, size, dst))
        {
            std::atomic_thread_fence (std::memory_order_acquire);
            readHead = (r + size) & sizeMask;
            return size;
        }
        else 
        {
            return 0;
        }
    }

    /** Skip data in the RingBuffer (advance read head without reading). */
    inline uint32_t skip (uint32_t size)
    {
        const uint32_t r = readHead;
        const uint32_t w = writeHead;
        if (readSpaceInternal (r, w) < size)
            return 0;

        std::atomic_thread_fence (std::memory_order_acquire);
        readHead = (r + size) & sizeMask;
        return size;
    }

    /** Write data to the RingBuffer. */
    inline uint32_t write (uint32_t size, const void* src)
    {
        const uint32_t r = readHead;
        const uint32_t w = writeHead;
        if (writeSpaceInternal (r, w) < size)
            return 0;

        if (w + size <= bufferSize)
        {
            memcpy(&buffer[w], src, size);
            std::atomic_thread_fence (std::memory_order_release);
            writeHead = (w + size) & sizeMask;
        }
        else 
        {
            const uint32_t this_size = bufferSize - w;
            assert (this_size < size);
            assert (w + this_size <= bufferSize);
            memcpy (&buffer[w], src, this_size);
            memcpy (&buffer[0], (const char*)src + this_size, size - this_size);
            std::atomic_thread_fence (std::memory_order_release);
            writeHead = size - this_size;
        }

        return size;
    }

private:
    mutable uint32_t writeHead;
    mutable uint32_t readHead;
    uint32_t bufferSize;
    uint32_t sizeMask;
    char* buffer;

    inline uint32_t writeSpaceInternal (uint32_t r, uint32_t w) const
    {
        if (r == w) {
            return bufferSize - 1;
        } else if (r < w) {
            return ((r - w + bufferSize) & sizeMask) - 1;
        } else {
            return (r - w) - 1;
        }
    }

    inline uint32_t readSpaceInternal (uint32_t r, uint32_t w) const
    {
        if (r < w) {
            return w - r;
        } else {
            return (w - r + bufferSize) & sizeMask;
        }
    }

    inline uint32_t peekInternal (uint32_t r, uint32_t w,
                                   uint32_t size, void* dst) const
    {
        if (readSpaceInternal (r, w) < size) {
            return 0;
        }

        if (r + size < bufferSize)
        {
            memcpy (dst, &buffer[r], size);
        } 
        else
        {
            const uint32_t first_size = bufferSize - r;
            memcpy (dst, &buffer[r], first_size);
            memcpy ((char*)dst + first_size, &buffer[0], size - first_size);
        }

        return size;
    }
};

}

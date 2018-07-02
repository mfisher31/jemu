/*
    This file is part of Jemu

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

#include <jemu/plugin.h>
#include "JuceHeader.h"

class GameCore
{
public:
    virtual ~GameCore() { }
    virtual void reset() =0;
    virtual void prepare() =0;
    virtual void release() =0;
    virtual void tick() =0;
    virtual bool load (const char*) =0;
    virtual void readAudio (float*, const int) { }
    virtual uint8_t* getVideoBuffer() const { return nullptr; }
    
    inline virtual void buttonPress (const uint32_t button, const bool pressed)
    {
        if (pressed)
            buttonPressed (button);
        else
            buttonReleased (button);
    }

    virtual void buttonReleased (const uint32_t) { }
    virtual void buttonPressed (const uint32_t) { }

    int getWidth()  const { return 256; }
    int getHeight() const { return 240; }

protected:
    GameCore() { }
};

class GameCoreInstance : public GameCore
{
public:
    explicit GameCoreInstance (const JemuDescriptor* d, JemuHandle h) 
        : desc (*d), handle (h)
    {
        jassert (nullptr != handle);
        memset (&core, 0, sizeof (JemuGameCore));
        if (const void* data = desc.extension (JEMU_GAME_CORE))
            memcpy (&core, data, sizeof (JemuGameCore));

        jassert (nullptr != core.button_press);
        jassert (nullptr != core.load);
        jassert (nullptr != core.read_audio);
        jassert (nullptr != core.reset);
        jassert (nullptr != core.tick);
        jassert (nullptr != core.video_frame);
    }

    ~GameCoreInstance() noexcept
    { 
        if (handle != nullptr && desc.destroy != nullptr)
        {
            JemuHandle handleRef = handle;
            handle = nullptr;
            desc.destroy (handleRef);
        }
    }

    void prepare()      override { if (core.prepare != nullptr)  core.prepare (handle); }
    void release()      override { if (core.release != nullptr)  core.release (handle); }
    void reset()        override { if (core.reset != nullptr)    core.reset (handle); }
    void tick()         override { if (core.tick != nullptr)     core.tick (handle); }

    bool load (const char* path) override {
        return (core.load != nullptr) ? core.load (handle, path) : false;
    }

    uint8_t* getVideoBuffer() const override
    {
        return core.video_frame != nullptr ? core.video_frame (handle)
                                           : nullptr; 
    }

    void readAudio (float* out, const int nframes) override { 
        if (core.read_audio != nullptr) core.read_audio (handle, out, nframes); 
    }
    
    void buttonPress (const uint32_t button, const bool pressed) override {
        core.button_press (handle, button, pressed);
    }

private:
    const JemuDescriptor desc;
    JemuGameCore core;
    JemuHandle handle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GameCoreInstance);
};

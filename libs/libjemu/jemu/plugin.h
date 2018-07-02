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

#ifndef JEMU_PLUGIN_H
#define JEMU_PLUGIN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JEMU_PREFIX "org.jemu."
#define JEMU_GAME_CORE          JEMU_PREFIX "GameCore"
#define JEMU_GAME_PAD           JEMU_PREFIX "GamePad"
#define JEMU_GAME_PAD_SOURCE    JEMU_PREFIX "GamePadSource"
#define JEMU_MFI                JEMU_PREFIX "MFI"
#define JEMU_NESTOPIA           JEMU_PREFIX "Nestopia"

typedef void* JemuHandle;
typedef void* JemuGamePadHandle;
typedef void* JemuGamePadSourceHandle;

typedef struct _JemuExtension {
    /** Unique identifier for this feature */
    const char* ID;

    /** Opaque feature data */
    void* data;
} JemuExtension;

typedef struct _JemuDescriptor {
    /** Globally unique identifier for this plugin */
    const char* ID;

    /** Allocate plugin instance */
    JemuHandle (*instantiate)(const char* bundle_path);

    /** Destroy/clean-up resources */
    void (*destroy)(JemuHandle handle);

    /** Returns extension data */
    const void * (*extension)(const char * identifier);
} JemuDescriptor;

typedef struct _JemuGameCore {
    /** prepare for emulation */
    void (*prepare)(JemuHandle);

    /** Release resources allocated in prepare */
    void (*release)(JemuHandle);

    /** Process the next emulation frame */
    void (*tick)(JemuHandle);

    /** Reset the emulator. e.g. press reset button on a console */
    void (*reset)(JemuHandle);

    /** Load a rom file */
    bool (*load)(JemuHandle, const char* rom_path);

    /** Read the next bit of audio */
    void (*read_audio)(JemuHandle, float *output, uint32_t nframes);

    /** Read the current video frame */
    uint8_t* (*video_frame)(JemuHandle);

    /** Button Press */
    void (*button_press)(JemuHandle, const uint32_t button, const bool pressed);
} JemuGameCore;

typedef struct _JemuGamePadSource {
    JemuGamePadSourceHandle handle;
    void (*connected)(JemuGamePadSourceHandle, JemuGamePadHandle);
    void (*disconnected)(JemuGamePadSourceHandle, JemuGamePadHandle);
} JemuGamePadSource;

typedef struct _JemuGamePad {
    /** Start or stop scanning for connected controllers */
    void (*discover)(JemuHandle, bool scan);
    const char* (*name)(JemuHandle, JemuGamePadHandle);
} JemuGamePad;

// -----

typedef struct _JemuGameCoreDescriptor {
    /** Unique identifier for this interface */
    const char* ID;

    /** Allocate plugin instance. Note: heavy lifting should be done in 
        the prepare method */
    JemuHandle (*instantiate)(const char* bundle_path);

    /** Destroy the plugin instance */
    void (*destroy)(JemuHandle);

    /** Prepare for emulation */
    void (*prepare)(JemuHandle);

    /** Free resources allocated in prepare */
    void (*release)(JemuHandle);

    /** Process the next emulation frame */
    void (*tick)(JemuHandle);

    /** Reset the emulator. e.g. press reset button on a console */
    void (*reset)(JemuHandle);

    /** Load a rom file */
    bool (*load)(JemuHandle, const char* rom_path);

    /** Read the next bit of audio */
    void (*read_audio_test)(JemuHandle, float *output, uint32_t nframes);

    /** Read the current video frame */
    uint8_t* (*video_frame_test)(JemuHandle);

    /** Button Press */
    void (*button_press)(JemuHandle, const uint32_t button, const bool pressed);
} JemuGameCoreDescriptor;

typedef struct _JemuGamePadDescriptor {
    /** Unique identifier for this interface */
    const char* ID;

    /** Allocate plugin instance. Note: heavy lifting should be done in 
        the prepare method */
    JemuHandle (*instantiate)(const char* bundle_path);

    /** Start or sop scanning for connected controllers */
    void (*discover)(JemuHandle, bool scan);


} JemuGamePadDescriptor;

#ifdef __cplusplus
 #define JEMU_SYMBOL_EXTERN extern "C"
#else
 #define JEMU_SYMBOL_EXTERN
#endif

#ifdef _WIN32
 #define JEMU_SYMBOL_EXPORT JEMU_SYMBOL_EXTERN __declspec(dllexport)
#else
 #define JEMU_SYMBOL_EXPORT JEMU_SYMBOL_EXTERN __attribute__((visibility("default")))
#endif

JEMU_SYMBOL_EXPORT
const JemuGameCoreDescriptor* jemu_gamecore_descriptor (const uint32_t index);
typedef const JemuGameCoreDescriptor* (*JemuGameCoreFunction)(const uint32_t);

JEMU_SYMBOL_EXPORT
const JemuDescriptor* jemu_descriptor (const uint32_t index);

typedef const JemuDescriptor* (*JemuDescriptorFunction)(const uint32_t);

#ifdef __cplusplus
} /* extern "C" */

#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace jemu {

typedef JemuDescriptor Descriptor;
typedef JemuGameCoreDescriptor GameCoreDescriptor;
typedef std::initializer_list<std::string> ExtensionList;
typedef const void* (*ExtensionDataFunction)(const char*);

struct DescriptorList : public std::vector<Descriptor> {
    inline ~DescriptorList() {
        for (const auto& desc : *this)
            std::free ((void*) desc.ID);
    }
};

/** @internal */
inline static DescriptorList& descriptors()
{
    static DescriptorList _descriptors;
    return _descriptors;
}

struct GameCoreExtension
{
    virtual void prepareGameCore() { }
    virtual void releaseGameCoreResources() { }
    virtual void buttonPress (const uint32_t button, const bool pressed) { }
    virtual bool loadRom (const char* path) { return false; }
    virtual void resetGameCore() { }
    virtual void readAudio (float* output, int sampleCount) { }
    virtual void tick() { }
    virtual uint8_t* getVideoBuffer() const { return nullptr; }
};

struct GamePadExtension
{
    GamePadExtension() = default;
    virtual ~GamePadExtension() = delete;
    virtual void discoverGamePads (const bool scan) { }
};

/** C++ bindings to the C interface */
template<class Instance>
class Plugin
{
public:
    Plugin() = default;
    virtual ~Plugin() = delete;

    inline static uint32_t registerDescriptor (const std::string& _identifier,
                                               const ExtensionList& _extensions = { },
                                               ExtensionDataFunction extensionData = nullptr)
    {
        Descriptor desc;
        memset (&desc, 0, sizeof (Descriptor));
        desc.ID             = strdup (_identifier.c_str());
        desc.instantiate    = &PluginType::instantiate;
        desc.destroy        = &PluginType::destroy;
        desc.extension      = &PluginType::extension;

        ExtensionDataFunction getExtensionData = extensionData != nullptr 
            ? extensionData : &PluginType::extensionData;
        for (const std::string& ext : _extensions)
            extensions()[ext] = getExtensionData (ext.c_str());

        jemu::descriptors().push_back (desc);
        return jemu::descriptors().size() - 1;
    }

private:
    typedef Plugin<Instance> PluginType;
    typedef std::map<std::string, const void*> ExtensionMap;

    inline static ExtensionMap& extensions() {
        static ExtensionMap _extensions;
        return _extensions;
    }

    /** default extension data getter */
    inline static const void* extensionData (const char* identifier) {
        void* data = nullptr;

        if (strcmp (identifier, JEMU_GAME_CORE) == 0)
        {
            typedef PluginType::GameCoreImpl Impl;
            static JemuGameCore _gamecore;
            memset (&_gamecore, 0, sizeof (JemuGameCore));
            _gamecore.prepare       = &Impl::prepare;
            _gamecore.release       = &Impl::release;
            _gamecore.button_press  = &Impl::buttonPress;
            _gamecore.load          = &Impl::load;
            _gamecore.read_audio    = &Impl::readAudio;
            _gamecore.reset         = &Impl::reset;
            _gamecore.tick          = &Impl::tick;
            _gamecore.video_frame   = &Impl::videoFrame;
            data = (void*) &_gamecore;
        }
        else if (strcmp (identifier, JEMU_GAME_PAD) == 0)
        {
            typedef PluginType::GamePadImpl Impl;
            static JemuGamePad _gamepad;
            memset (&_gamepad, 0, sizeof (JemuGamePad));
            _gamepad.discover       = &Impl::discover;
            _gamepad.name           = &Impl::name;
            data = (void*) &_gamepad;
        }

        return data;
    }

    // Base Plugin
    inline static JemuHandle instantiate (const char* bundlePath) {
        std::unique_ptr<Instance> instance (Instance::create (bundlePath));
        return static_cast<JemuHandle> (instance.release());
    }

    inline static void destroy (JemuHandle handle) {
        delete static_cast<Instance*> (handle);
    }

    inline static const void* extension (const char* identifier) {
        auto iter = extensions().find (identifier);
        return iter != extensions().end() ? (*iter).second : nullptr;
    }

    struct GameCoreImpl
    {
        inline static void prepare (JemuHandle handle) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->prepareGameCore();
        }

        inline static void release (JemuHandle handle) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->releaseGameCoreResources();
        }

        inline static void buttonPress (JemuHandle handle, const uint32_t button, const bool pressed) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->buttonPress (button, pressed);
        }

        inline static bool load (JemuHandle handle, const char* path) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    return core->loadRom (path);
            return false;
        }

        inline static void readAudio (JemuHandle handle, float* output, uint32_t sampleCount) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->readAudio (output, static_cast<int> (sampleCount));
        }

        inline static void reset (JemuHandle handle) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->resetGameCore();
        }

        inline static void tick (JemuHandle handle) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    core->tick();
        }

        inline static uint8_t* videoFrame (JemuHandle handle) {
            if (auto* instance = static_cast<Instance*> (handle))
                if (auto* core = dynamic_cast<GameCoreExtension*> (instance))
                    return core->getVideoBuffer();
            return nullptr;
        }
    };

    struct GamePadImpl
    {
        inline static void discover (JemuHandle handle, bool scan) {
            if (auto *instance = static_cast<Instance*> (handle))
                if (auto* pad = dynamic_cast<GamePadExtension*> (instance))
                    pad->discoverGamePads (scan);
        }

        inline static const char* name (JemuHandle handle, JemuGamePadHandle gamecore) {
            return "";
        }
    };
};

}

#define JEMU_REGISTER_PLUGIN(t, i, e) static uint32_t __t = jemu::Plugin<t>::registerDescriptor(i, e); \
    JEMU_SYMBOL_EXPORT const JemuDescriptor* jemu_descriptor (const uint32_t index) { \
        return index < jemu::descriptors().size() \
            ? &jemu::descriptors()[index] : NULL; }

#endif // __cplusplus
#endif // header

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

#include <jemu/plugin.h>
#include "MainComponent.h"
#include "GameCore.h"
#include "GamePad.h"

#define DEFAULT_GAME "/path/to/game.nes"

static String getPluginBundlePath (const String& name)
{
   #if JUCE_MAC
    return File::getSpecialLocation(File::invokedExecutableFile)
        .getParentDirectory().getParentDirectory()
        .getChildFile("Frameworks/plugins").getChildFile(name)
        .withFileExtension("jemu")
        .getFullPathName();
   #else
    return File::getSpecialLocation(File::invokedExecutableFile)
        .getParentDirectory().getChildFile("plugins").getChildFile(name)
        .withFileExtension("emu");
   #endif
}

class PluginBundle
{
public:
    PluginBundle (const String& _bundlePath)
        : bundlePath (_bundlePath)
    { }

    static const String binaryExtension()
    {
       #if JUCE_MAC
        return ".dylib";
       #elif JUCE_WINDOWS
        return ".dll";
       #elif JUCE_LINUX
        return ".so";
       #endif
    }

    bool isOpen() const { return libraryOpen; }

    bool open()
    {
        close();
        if (! libraryOpen)
        {
            DBG("[emu] open plugin: " << File(bundlePath).getFileName());
            DBG("[emu] " << bundlePath);
            File libraryFile (bundlePath);
            String fileName = libraryFile.getFileNameWithoutExtension(); 
            fileName << binaryExtension();
            libraryFile = libraryFile.getChildFile (fileName);
            libraryOpen = library.open (libraryFile.getFullPathName().toRawUTF8());
            if (libraryOpen)
            {
                uint32 i = 0; 
                for (;;)
                {
                    if (const auto* desc = getDescriptor (i))
                        descriptors.add (desc);
                    else
                        break;
                    ++i;
                }
            }
        }

        return libraryOpen;
    }

    void close()
    {
        if (! libraryOpen)
            return;
        libraryOpen = false;
        descriptors.clearQuick();
        library.close();        
    }

    GameCoreInstance* instantiateGameCore (const String& identifier)
    {
        if (! isOpen())
            return nullptr;

        for (const auto* desc : descriptors)
            if (strcmp (identifier.toRawUTF8(), desc->ID) == 0)
                return createGameCore (desc);

        return nullptr;
    }

private:
    const String bundlePath;
    DynamicLibrary library;
    bool libraryOpen;
    Array<const JemuDescriptor*> descriptors;
    JemuDescriptorFunction descriptorFunction = nullptr;

    GameCoreInstance* createGameCore (const JemuDescriptor* desc)
    {
        jassert (desc != nullptr && desc->instantiate != nullptr && desc->extension != nullptr);
        if (desc == nullptr || desc->instantiate == nullptr || desc->extension == nullptr)
            return nullptr;
        if (JemuHandle instance = desc->instantiate (bundlePath.toRawUTF8()))
            return new GameCoreInstance (desc, instance);
        return nullptr;
    }

    GamePadSource* createGamePadSource (const JemuDescriptor* desc) {
        return nullptr;
    }

    const JemuDescriptor* getDescriptor (const uint32 index)
    {
        if (descriptorFunction == nullptr)
            descriptorFunction = (JemuDescriptorFunction) library.getFunction ("jemu_descriptor");
        return descriptorFunction != nullptr ? descriptorFunction (index) : nullptr;
    }
};

class GamePlayEngine : private HighResolutionTimer,
                       public AudioIODeviceCallback,
                       public GamePadManager::Listener
{
public:
    GamePlayEngine() { videoImage = Image (Image::PixelFormat::RGB, 256, 240, true); }
    ~GamePlayEngine() noexcept { stopTimer(); }

    void loadPlugin (const String& name)
    {
        const bool wasRunning = isTimerRunning();
        stop();

        width = height = 0;
        core.reset (nullptr);
        bundle.reset (new PluginBundle (getPluginBundlePath (name)));

        if (bundle != nullptr && bundle->open())
            core.reset (bundle->instantiateGameCore (JEMU_NESTOPIA));

        if (core != nullptr)
        {
            core->prepare();
            width = core->getWidth();
            height = core->getHeight();
            videoImage = Image (Image::PixelFormat::RGB, width, height, true);
            if (core->load (DEFAULT_GAME))
                core->reset();
        }

        if (wasRunning)
            start();
    }

    void start()
    {
        if (isTimerRunning())
            stop();
        startTimer (1000 / 60);
    }

    void stop() { stopTimer(); }

    Image createImageTemplate() const
    {
        Image image (videoImage.getFormat(), videoImage.getWidth(), videoImage.getHeight(), true);
        return image;
    }

    void sendNativeButtonPress (const uint32_t button)
    {
        core->buttonPress (button, true);
    }

    void sendNativeButtonRelease (const uint32_t button)
    {
        core->buttonPress (button, false);
    }

    void copyVideoImage (Image image)
    {
        ScopedLock sl (coreLock);
        Image::BitmapData src (videoImage, Image::BitmapData::readOnly);
        Image::BitmapData dst (image, Image::BitmapData::readWrite);

        if (src.width == dst.width &&
            src.height == src.height &&
            src.pixelFormat == src.pixelFormat &&
            src.pixelStride == src.pixelStride)
        {
            memcpy (dst.data, src.data, src.width * src.height * src.pixelStride);
        }
    }

    int getWidth()  const { return width; }
    int getHeight() const { return height; }

    void audioDeviceIOCallback (const float** inputs, int numInputs, 
                                float** outputs, int numOutputs, int numSamples) override
    {
        bool haveCore = false;

        {
            ScopedLock sl (coreLock);
            haveCore = core != nullptr;
        }

        for (int channel = 0; channel < numOutputs; ++channel)
        {
            // FloatVectorOperations::clear (outputs[channel], numSamples);
            if (! haveCore)
                continue;

            if (channel == 0)
            {
                core->readAudio (outputs [channel], numSamples);
            }
            else
            {
                FloatVectorOperations::copy (outputs[channel], outputs[0], numSamples);
            }
        }
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override { }

    void audioDeviceStopped() override { }

    void audioDeviceError (const String& errorMessage) override
    {
        DBG("[emu] audio device error: " << errorMessage);
    }

private:
    std::unique_ptr<PluginBundle> bundle;
    std::unique_ptr<GameCore> core;
    CriticalSection coreLock;
    Image videoImage;

    int width = 0;
    int height = 0;

    friend class HighResolutionTimer;
    void hiResTimerCallback() override
    {
        ScopedLock sl (coreLock);
        if (nullptr == core)
            return;
        core->tick();
        renderImage (core.get());
    }

    void renderImage (GameCore* c)
    {
        const uint8* buffer = (const uint8*) c->getVideoBuffer();
        if (videoImage.isNull() || !videoImage.isValid())
            return;
        Image::BitmapData bitmap (videoImage, Image::BitmapData::readWrite);
        
        if (buffer != nullptr)
        {
            for (int i = 0; i < bitmap.width * bitmap.height * 4; i += 4)
            {
                bitmap.data[i + 0] = buffer [i + 0];
                bitmap.data[i + 1] = buffer [i + 1];
                bitmap.data[i + 2] = buffer [i + 2];
                bitmap.data[i + 3] = 255;
            }
        }
    }

    int nestopiaControl (const int control)
    {
        if (control == GamePad::START)
            return GamePad::buttonX;
        else if (control == GamePad::buttonX)
            return GamePad::buttonB;
        return control;
    }

    // friend class GamePadManager;
    void handleGamePadEvent (const GamePadEvent& event) override
    {
        switch (event.type)
        {
            case GamePadEvent::ButtonPress:
            {
                sendNativeButtonPress (nestopiaControl (event.control));
            } break;

            case GamePadEvent::ButtonRelease:
            {
                sendNativeButtonRelease (nestopiaControl (event.control));
            } break;

            case GamePadEvent::ValueChange:
                break;
        }
    }
};

class GameDisplayComponent : public Component,
                             public Timer
{
public:
    GameDisplayComponent (GamePlayEngine& e) : engine (e)
    {
        setSize (256 * 2, 240 * 2);
        startTimerHz (30);
        setWantsKeyboardFocus (true);
        setMouseClickGrabsKeyboardFocus (true);
        for (int i = 0; i < 10; ++i)
            buttonState.set (i, false);
    }

    void timerCallback() override
    {
        grabKeyboardFocus();
        update();
    }

    void paint (Graphics& g) override
    {
        if (videoImage.isNull() || !videoImage.isValid())
            return paintNoImage (g);
        
        g.fillAll (Colours::black);
        g.drawImageWithin (videoImage, 0, 0, getWidth(), getHeight(),
                           RectanglePlacement::centred, false);
    }

    void resized() override { }

    void update()
    {
        if (videoImage.isNull() || !videoImage.isValid())
            videoImage = engine.createImageTemplate();
        if (videoImage.isNull() || !videoImage.isValid())
            return;
        engine.copyVideoImage (videoImage);
        repaint();
    }

    bool keyPressed (const KeyPress& key) override
    {
        bool result = true;
        bool send = false;
        int index = -1;

        if (!buttonState[0] && key.getKeyCode() == KeyPress::upKey)
        {
            buttonState.set (0, true); 
            send = true;
            index = 0;
        }

        else if (! buttonState[1] && key.getKeyCode() == KeyPress::downKey)
        {
            buttonState.set (1, true); 
            send = true; 
            index = 1;
        }
        
        else if (! buttonState[2] && key.getKeyCode() == KeyPress::leftKey)
        {
            buttonState.set (2, true); 
            send = true; 
            index = 2;
        }

        else if (! buttonState[3] && key.getKeyCode() == KeyPress::rightKey)
        {
            buttonState.set (3, true); 
            send = true; 
            index = 3;
        }

        // A
        else if (! buttonState[4] && (key.getKeyCode() == 'a' || key.getKeyCode() == 'A'))
        {
            buttonState.set (4, true); 
            send = true; 
            index = 4;
        }

        // B
        else if (! buttonState[5] && (key.getKeyCode() == 's' || key.getKeyCode() == 'S'))
        {
            buttonState.set (5, true); 
            send = true; 
            index = 5;
        }

        // START
        else if (! buttonState[6] && (key.getKeyCode() == 'w' || key.getKeyCode() == 'W'))
        {
            buttonState.set (6, true); 
            send = true; 
            index = 6;
        }

        // SELECT
        else if (! buttonState[7] && (key.getKeyCode() == 'q' || key.getKeyCode() == 'Q'))
        {
            buttonState.set (7, true); 
            send = true; 
            index = 7;
        }

        else
        {
            result = send = false;
            index = -1;
        }

        if (result && send && index >= 0)
        {
            engine.sendNativeButtonPress (index);
        }

        return result;
    }

    bool keyStateChanged (bool isDown) override
    {
        if (isDown)
            return Component::keyStateChanged (isDown);
        
        Array<int> indexes;
        bool result = true;
        bool send   = false;

        if (buttonState [0] && !KeyPress::isKeyCurrentlyDown (KeyPress::upKey))
        {
            buttonState.set (0, false); 
            send = true; 
            indexes.add (0);
        }
        
        if (buttonState [1] && !KeyPress::isKeyCurrentlyDown (KeyPress::downKey))
        {
            buttonState.set (1, false); 
            send = true; 
            indexes.add(1);
        }

        if (buttonState [2] && !KeyPress::isKeyCurrentlyDown (KeyPress::leftKey))
        {
            buttonState.set (2, false); 
            send = true; 
            indexes.add (2);
        }

        if (buttonState [3] && !KeyPress::isKeyCurrentlyDown (KeyPress::rightKey))
        {
            buttonState.set (3, false); 
            send = true; 
            indexes.add (3);
        }
        
        if (buttonState [4] && !KeyPress::isKeyCurrentlyDown('a') && !KeyPress::isKeyCurrentlyDown('A'))
        {
            buttonState.set (4, false); 
            send = true; 
            indexes.add (4);
        }
        
        if (buttonState [5] && !KeyPress::isKeyCurrentlyDown('s') && !KeyPress::isKeyCurrentlyDown('S'))
        {
            buttonState.set (5, false); 
            send = true; 
            indexes.add(5);
        }

        if (buttonState [6] && !KeyPress::isKeyCurrentlyDown('w') && !KeyPress::isKeyCurrentlyDown('W'))
        {
            buttonState.set (6, false); 
            send = true; 
            indexes.add (6);
        }

        if (buttonState [7] && !KeyPress::isKeyCurrentlyDown ('q') && !KeyPress::isKeyCurrentlyDown ('Q'))
        {
            buttonState.set (7, false); 
            send = true; 
            indexes.add (7);
        }

        if (indexes.size() > 0)
        {
            for (const int& index : indexes)
                engine.sendNativeButtonRelease (index);
        }
        else
        {
            result = false;
        }

        return result;
    }

private:
    GamePlayEngine& engine;
    Image videoImage;
    HashMap<int, bool> buttonState;

    void paintNoImage (Graphics& g) { g.fillAll (Colours::black); }
};

class EmulatorApplication  : public JUCEApplication,
                             public AsyncUpdater
{
public:
    EmulatorApplication() { }

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    void initialise (const String& commandLine) override
    {
        setupAudioDevices();
        engine.loadPlugin ("nestopia");
        mainWindow.reset (new MainWindow (getApplicationName(), engine));
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        gamepads.addListener (&engine);
        gamepads.addDefaultSources();
        engine.start();
    }

    void shutdown() override
    {
        gamepads.removeListener (&engine);
        mainWindow = nullptr;
        shutdownAudio();
        engine.stop();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override { }

    class MainWindow : public DocumentWindow
    {
    public:
        MainWindow (String name, GamePlayEngine& engine)
            : DocumentWindow (name, Desktop::getInstance().getDefaultLookAndFeel()
                                        .findColour (ResizableWindow::backgroundColourId),
                                                    DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new GameDisplayComponent (engine), true);
            centreWithSize (getWidth(), getHeight());
            setResizable (true, false);
            setVisible (true);
        }
        
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<PluginBundle> plugin;
    GamePlayEngine engine;
    AudioDeviceManager devices;
    GamePadManager gamepads;

    void setupAudioDevices()
    {
        devices.addAudioCallback (&engine);
        devices.initialiseWithDefaultDevices (0, 2);
        AudioDeviceManager::AudioDeviceSetup setup;
        devices.getAudioDeviceSetup (setup);
        setup.sampleRate = 48000.0;
        devices.setAudioDeviceSetup (setup, true);
    }

    void shutdownAudio()
    {
        devices.removeAudioCallback (&engine);
        devices.closeAudioDevice();
    }
};

START_JUCE_APPLICATION (EmulatorApplication)

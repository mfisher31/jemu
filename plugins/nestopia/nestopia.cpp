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

#include <fstream>
#include <jemu/plugin.h>
#include <jemu/ringbuffer.h>

#include "core/NstBase.hpp"
#include "core/NstMachine.hpp"
#include "core/api/NstApiEmulator.hpp"
#include "core/api/NstApiMachine.hpp"
#include "core/api/NstApiCartridge.hpp"
#include "core/api/NstApiVideo.hpp"
#include "core/api/NstApiSound.hpp"
#include "core/api/NstApiUser.hpp"
#include "core/api/NstApiCheats.hpp"
#include "core/api/NstApiRewinder.hpp"
#include "core/api/NstApiMovie.hpp"
#include "core/api/NstApiFds.hpp"
#include "nes_ntsc/nes_ntsc.inl"

#include "JuceHeader.h"

#define SAMPLERATE 48000

static int nstControlValues[] = {
    Nes::Api::Input::Controllers::Pad::UP,      // 0
    Nes::Api::Input::Controllers::Pad::DOWN,    // 1
    Nes::Api::Input::Controllers::Pad::LEFT,    // 2
    Nes::Api::Input::Controllers::Pad::RIGHT,   // 3
    Nes::Api::Input::Controllers::Pad::A,       // 4
    Nes::Api::Input::Controllers::Pad::B,       // 5
    Nes::Api::Input::Controllers::Pad::START,   // 6
    Nes::Api::Input::Controllers::Pad::SELECT   // 7
};

static Nes::Api::Video::RenderState::Filter nstFilters[2] =
{
    Nes::Api::Video::RenderState::FILTER_NONE,
    Nes::Api::Video::RenderState::FILTER_NTSC
};

static int nstWidths[2] =
{
    Nes::Api::Video::Output::WIDTH,
    Nes::Api::Video::Output::NTSC_WIDTH
};

static int nstHeights[2] =
{
    Nes::Api::Video::Output::HEIGHT,
    Nes::Api::Video::Output::HEIGHT
};

class NestopiaGameCore : public jemu::GameCoreExtension
{
public:
    NestopiaGameCore (const String& bundle)
        : bundlePath (bundle),
          audioRingBuffer (sizeof(uint16) * 2)
    {
        nesSound.reset (new Nes::Api::Sound::Output());
        nesVideo.reset (new Nes::Api::Video::Output());
        controls.reset (new Nes::Api::Input::Controllers());
        emu.reset (new Nes::Api::Emulator());
    }
    
    static NestopiaGameCore* create (const char* bundlePath) {
        return new NestopiaGameCore (bundlePath);
    }

    ~NestopiaGameCore() noexcept
    {
        nesSound.reset();
        nesVideo.reset();
        controls.reset();
        emu.reset();
    }

    void reset()
    {
        DBG("[emu] nestopia: reset");
        Nes::Api::Machine machine (*emu);
        machine.Reset (true);
        
        if (machine.Is (Nes::Api::Machine::DISK))
        {
            Nes::Api::Fds fds (*emu);
            fds.EjectDisk();
            fds.InsertDisk(0, 0);
        }
    }

    void releaseGameCoreResources() override
    {
        Nes::Api::Machine machine (*emu);
        // machine.Power(false);
        machine.Unload(); // this allows FDS to save
    }

    double getSampleRate() const { return (double) SAMPLERATE; }
    int getNumAudioChannels() const { return 1; }

    uint8_t* getVideoBuffer() const override
    {
        nesVideo->pixels = (void*) videoBuffer.getData();
        nesVideo->pitch  = width * 4;
        return videoBufferSize > 0 ? videoBuffer.getData() : nullptr;
    }

    uint8_t* videoFrame() const { return getVideoBuffer(); }
    uint32 getFrameInterval() const
    {
        Nes::Api::Machine machine (*emu);
        return (machine.GetMode() == Nes::Api::Machine::NTSC)
            ? Nes::Api::Machine::CLK_NTSC_DOT / Nes::Api::Machine::CLK_NTSC_VSYNC // 60.0988138974
            : Nes::Api::Machine::CLK_PAL_DOT / Nes::Api::Machine::CLK_PAL_VSYNC;  // 50.0069789082
    }

    void prepareGameCore() override
    {
        Nes::Api::Machine machine (*emu);
        Nes::Api::Cartridge::Database database (*emu);

        if (! database.IsLoaded())
        {
            const File databaseFile (File(bundlePath).getChildFile ("database.xml"));
            if (databaseFile.existsAsFile())
            {
                DBG("[emu] nestopia: loading database");
                std::ifstream databaseStream (databaseFile.getFullPathName().toRawUTF8(), 
                    std::ifstream::in | std::ifstream::binary);
                database.Load(databaseStream);
                database.Enable(true);
                databaseStream.close();
            }
        }

        if (database.IsLoaded())
        {
            DBG("[emu] nestopia: database loaded");
            Nes::Api::Input(*emu).AutoSelectControllers();
            Nes::Api::Input(*emu).AutoSelectAdapter();
        }
        else
        {
            DBG("[emu] nestopia: database not loaded");
            Nes::Api::Input(*emu).ConnectController (0, Nes::Api::Input::PAD1);
        }

        machine.SetMode (machine.GetDesiredMode());
        prepareVideo (isNtscEnabled() ? 1 : 0);
        prepareAudio();

        DBG("[emu] nestopia: setup");
    }

    void readAudio (float* buffer, int numSamples) override
    {
        for (int f = 0; f < numSamples; ++f)
            buffer[f] = 0.f;
        if (audioRingBuffer.readSpace() < static_cast<uint32> (sizeof(uint16) * numSamples))
            return;
        audioRingBuffer.read (static_cast<uint32> (numSamples) * sizeof (uint16),
            soundReadBuffer.getData());
        AudioDataConverters::convertFormatToFloat (AudioDataConverters::int16LE,
            soundReadBuffer.getData(), buffer, numSamples);
    }

    void setCheat (const String& code, const bool enabled)
    {
        // TODO: Remove any spaces, newlines and whitespace
       
        Nes::Api::Cheats cheater (*emu);
        Nes::Api::Cheats::Code ggCode;
        
        if (enabled)
            cheatMap.set (code, enabled);
        else
            cheatMap.remove (code);
        
        cheater.ClearCodes();
        
        // TODO: rewrite in c++
        // NSArray *multipleCodes = [[NSArray alloc] init];
        // // Apply enabled cheats found in dictionary
        // for (id key in cheatList)
        // {
        //     if ([[cheatList valueForKey:key] isEqual:@YES])
        //     {
        //         // Handle multi-line cheats
        //         multipleCodes = [key componentsSeparatedByString:@"+"];
        //         for (NSString *singleCode in multipleCodes) {
        //             const char *cCode = [singleCode UTF8String];
                    
        //             Nes::Api::Cheats::GameGenieDecode(cCode, ggCode);
        //             cheater.SetCode(ggCode);
        //         }
        //     }
        // }
    }

    bool loadRom (const char* romPath) override
    {
        DBG("[emu] nestopia: loading rom");
        DBG("[emu] nestopia: " << romPath);
        const File romFile (String::fromUTF8 (romPath));
        Nes::Result result;
        Nes::Api::Machine machine (*emu);
        Nes::Api::Cartridge::Database database (*emu);

        // if(! database.IsLoaded())
        // {
        //     NSString *databasePath = [[NSBundle bundleForClass:[self class]] pathForResource:@"NstDatabase" ofType:@"xml"];
        //     if(databasePath != nil)
        //     {
        //         DLog(@"Loading database");
        //         std::ifstream databaseStream(databasePath.fileSystemRepresentation, std::ifstream::in | std::ifstream::binary);
        //         database.Load(databaseStream);
        //         database.Enable(true);
        //         databaseStream.close();
        //     }
        // }

        // [self setRomPath:path];

        // void *userData = (__bridge void *)self;
        // Nes::Api::User::fileIoCallback.Set(doFileIO, userData);
        // Nes::Api::User::logCallback.Set(doLog, userData);
        // Nes::Api::Machine::eventCallback.Set(doEvent, userData);
        // Nes::Api::User::questionCallback.Set(doQuestion, userData);
        
        Nes::Api::Fds fds (*emu);
        const String biosFilePath = "";
        std::ifstream biosFile (biosFilePath.toRawUTF8(), std::ios::in | std::ios::binary);
        fds.SetBIOS (&biosFile);

        std::ifstream romStream (romFile.getFullPathName().toRawUTF8(), std::ios::in | std::ios::binary);
        result = machine.Load (romStream, Nes::Api::Machine::FAVORED_NES_NTSC, Nes::Api::Machine::ASK_PROFILE);

        if (NES_FAILED (result))
        {
            String errorDescription;
            switch(result)
            {
                case Nes::RESULT_ERR_INVALID_FILE :
                    errorDescription = "Invalid file";
                    break;
                case Nes::RESULT_ERR_OUT_OF_MEMORY :
                    errorDescription = "Out of memory";
                    break;
                case Nes::RESULT_ERR_CORRUPT_FILE :
                    errorDescription = "Corrupt file";
                    break;
                case Nes::RESULT_ERR_UNSUPPORTED_MAPPER :
                    errorDescription = "Unsupported mapper";
                    break;
                case Nes::RESULT_ERR_MISSING_BIOS :
                    errorDescription = "Can't find disksys.rom for FDS game.";
                    break;
                default :
                    errorDescription = "Unknown nestopia error ";
                    errorDescription << (int) result;
                    break;
            }

            Logger::writeToLog (String("[emu] nestopia: ") + errorDescription);
            return false;
        }

        machine.Power (true);
        
        if (machine.Is (Nes::Api::Machine::DISK))
            fds.InsertDisk (0, 0);

        return true;
    }

    void tick() override
    { 
        processFrame();
    }
        

    void buttonPress (const uint32_t button, const bool pressed) override
    { 
        return pressed ? buttonPressed (button) : buttonReleased (button);
    }

private:
    const String bundlePath;
    std::unique_ptr<Nes::Api::Sound::Output> nesSound;
    std::unique_ptr<Nes::Api::Video::Output> nesVideo;
    std::unique_ptr<Nes::Api::Input::Controllers> controls;
    std::unique_ptr<Nes::Api::Emulator> emu;
    CriticalSection audioLock, videoLock;
    HashMap<String, bool> cheatMap;
    HeapBlock<uint16> soundBuffer, soundReadBuffer;
    jemu::RingBuffer audioRingBuffer;
    HeapBlock<uint8> videoBuffer;
    int videoBufferSize = 0;
    uint32 bufFrameSize = 0;
    int width = 0;
    int height = 0;

    bool isNtscEnabled() const { return false; }

    void buttonPressed (const uint32_t index)
    {
        controls->pad[0].buttons |=  nstControlValues [index];
    }

    void buttonReleased (const uint32_t index)
    {
        controls->pad[0].buttons &= ~nstControlValues[index];
    }

    void prepareAudio()
    {
        Nes::Api::Sound sound (*emu);
        sound.SetSampleBits (16);
        sound.SetSampleRate (SAMPLERATE);
        sound.SetVolume (Nes::Api::Sound::ALL_CHANNELS, 100);
        sound.SetSpeaker (Nes::Api::Sound::SPEAKER_MONO);
        sound.SetSpeed (getFrameInterval());

        bufFrameSize = SAMPLERATE / getFrameInterval();
        soundBuffer.allocate (bufFrameSize * getNumAudioChannels(), true);
        soundReadBuffer.allocate (bufFrameSize * getNumAudioChannels(), true);
        audioRingBuffer.resize (5 * bufFrameSize * getNumAudioChannels() * sizeof (uint16));
        
        nesSound->samples[0]  = soundBuffer.getData();
        nesSound->length[0]   = bufFrameSize;
        nesSound->samples[1]  = nullptr;
        nesSound->length[1]   = 0;

        DBG("[emu] nestopia: loaded audio");
    }

    void prepareVideo (const int filter)
    {
        jassert (isPositiveAndBelow (filter, 2));
        Nes::Api::Video::RenderState *renderState = new Nes::Api::Video::RenderState();
        Nes::Api::Machine machine (*emu);
        Nes::Api::Video video (*emu);
        Nes::Api::Cartridge::Database database (*emu);

        machine.SetMode (Nes::Api::Machine::NTSC);

        width  = nstWidths [filter];
        height = nstHeights [filter];
        DBG("[emu] nestopia: video width: " << width << " height: " << height);

        renderState->bits.count  = 32;
        renderState->bits.mask.r = 0xFF0000;
        renderState->bits.mask.g = 0x00FF00;
        renderState->bits.mask.b = 0x0000FF;
        renderState->filter  = nstFilters [filter];
        renderState->width   = nstWidths [filter];
        renderState->height  = nstHeights [filter];

        if (NES_FAILED (video.SetRenderState (*renderState)))
        {
            DBG("[emu] nestopia: render state rejected");
            exit (0);
        }
        else
        {
            setDisplayMode (2);
            // video.SetSharpness(Nes::Api::Video::DEFAULT_SHARPNESS_RGB);
            // video.SetColorResolution(Nes::Api::Video::DEFAULT_COLOR_RESOLUTION_RGB);
            // video.SetColorBleed(Nes::Api::Video::DEFAULT_COLOR_BLEED_RGB);
            // video.SetColorArtifacts(Nes::Api::Video::DEFAULT_COLOR_ARTIFACTS_RGB);
            // video.SetColorFringing(Nes::Api::Video::DEFAULT_COLOR_FRINGING_RGB);
        }

        if (videoBufferSize <= 0)
        {
            videoBufferSize = width * height * 4;
            videoBuffer.allocate (videoBufferSize, true);
        }
        
        DBG("[emu] nestopia: loaded video");
    }

    void processFrame()
    {
        ScopedLock asl (audioLock);
        ScopedLock vsl (videoLock);
        emu->Execute (nesVideo.get(), nesSound.get(), controls.get());
        const uint32 requiredSize = getNumAudioChannels() * bufFrameSize * sizeof(uint16);
        if (requiredSize < audioRingBuffer.writeSpace())
            audioRingBuffer.write ((uint32) getNumAudioChannels() * bufFrameSize * sizeof (uint16),
                soundBuffer.getData());
    }

    void setDisplayMode (const int mode)
    {
        Nes::Api::Video video (*emu);
        
        switch (mode)
        {
            case 0:
                video.GetPalette().SetMode(Nes::Api::Video::Palette::MODE_YUV);
                video.SetDecoder(Nes::Api::Video::DECODER_CONSUMER);
                break;
                
            case 1:
                video.GetPalette().SetMode(Nes::Api::Video::Palette::MODE_YUV);
                video.SetDecoder(Nes::Api::Video::DECODER_ALTERNATIVE);
                break;
                
            case 2:
                video.GetPalette().SetMode(Nes::Api::Video::Palette::MODE_RGB);
                break;
                
            case 3:
                video.GetPalette().SetMode(Nes::Api::Video::Palette::MODE_YUV);
                video.SetDecoder(Nes::Api::Video::DECODER_CANONICAL);
                break;
                
            default:
                return;
                break;
        }
    }
};

JEMU_REGISTER_PLUGIN(NestopiaGameCore, JEMU_NESTOPIA, { JEMU_GAME_CORE });

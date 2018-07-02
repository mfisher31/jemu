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

#include <jemu/jemu.h>
#include <jemu/plugin.h>
#include "JuceHeader.h"

class GamePad
{
protected:
    GamePad() { }

public:
    enum ControlType
    {
        UP          = JEMU_CONTROL_UP, 
        DOWN        = JEMU_CONTROL_DOWN, 
        LEFT        = JEMU_CONTROL_LEFT, 
        RIGHT       = JEMU_CONTROL_RIGHT,
        A           = JEMU_CONTROL_A,
        B           = JEMU_CONTROL_B,
        X           = JEMU_CONTROL_X,
        Y           = JEMU_CONTROL_Y,
        L1          = JEMU_CONTROL_L1, 
        R1          = JEMU_CONTROL_R1, 
        L2          = JEMU_CONTROL_L2, 
        R2          = JEMU_CONTROL_L2,
        START       = JEMU_CONTROL_START, 
        SELECT      = JEMU_CONTROL_SELECT,

        buttonA = A, buttonB = B, buttonX = X, buttonY = Y
    };

    struct Listener
    {
        virtual void buttonPressed (GamePad*, const ControlType) { }
        virtual void buttonReleased (GamePad*, const ControlType) { }
        virtual void controlChanged (GamePad*, GamePad::ControlType, const int player, 
                                     const float value, const bool pressed) =0;
    };

    virtual ~GamePad() { }
    virtual String getName() const =0;
    virtual int getPlayer() const { return 0; }

    void addListener (Listener* listener) { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listeners.indexOf (listener)); }

protected:
    void sendButtonPress (GamePad::ControlType type) {
        for (auto* listener : listeners)
            listener->buttonPressed (this, type);
    }
    void sendButtonRelease (GamePad::ControlType type) {
        for (auto* listener : listeners)
            listener->buttonReleased (this, type);
    }

    void sendControlChange (GamePad::ControlType type, const float value, const bool pressed)
    {
        for (auto* listener : listeners)
            listener->controlChanged (this, type, getPlayer(), value, pressed);
    }

private:
    Array<Listener*> listeners;
};

class GamePadSource
{
public:
    struct Listener
    {
        virtual void gamepadConnected (GamePad*) =0;
        virtual void gamepadDisconnected (GamePad*) =0;
    };

    GamePadSource() { }
    virtual ~GamePadSource()
    {
        listeners.clear(); 
        pads.clear (true);
    }

    void addListener (Listener* l)      { listeners.add (l); }
    void removeListener (Listener* l)   { listeners.remove (listeners.indexOf (l)); }

protected:
    int getNumGamePads() const { return pads.size(); }
    GamePad* getGamePad (const int index) const
    { 
        return isPositiveAndBelow (index, pads.size())
            ? pads.getUnchecked (index) : nullptr;
    }

    void connectGamePad (GamePad* pad)
    {
        if (pads.addIfNotAlreadyThere (pad))
            for (auto* l : listeners)
                l->gamepadConnected (pad);
    }

    void disconnectGamePad (GamePad* pad)
    {
        if (! pads.contains (pad))
        {
            jassertfalse;
            return;
        }

        std::unique_ptr<GamePad> deleter (pad);
        pads.removeObject (pad, false);
        for (auto* const listener : listeners)
            listener->gamepadDisconnected (pad);
    }

private:
    friend class GamePadManager;
    Array<Listener*> listeners;
    OwnedArray<GamePad> pads;
    JemuGamePadSource source;
};

struct GamePadEvent
{
    enum Type { ButtonPress, ButtonRelease, ValueChange };
    int type;
    int control;
    int player;
    float value;
    bool pressed;
};

class GamePadManager : private GamePadSource::Listener,
                       private GamePad::Listener
{
public:
    struct Listener {
        virtual void handleGamePadEvent (const GamePadEvent&) =0;
    };

    GamePadManager();
    ~GamePadManager();

    void addSource (GamePadSource* source);
    void addDefaultSources();

    void addListener (Listener* listener)       { listeners.add (listener); }
    void removeListener (Listener* listener)    { listeners.remove (listeners.indexOf (listener)); }
    
private:
    OwnedArray<GamePadSource> sources;
    Array<Listener*> listeners;

    void gamepadConnected (GamePad*) override;
    void gamepadDisconnected (GamePad*) override;
    void buttonPressed (GamePad*, const GamePad::ControlType) override;
    void buttonReleased (GamePad*, const GamePad::ControlType) override;
    void controlChanged (GamePad* pad, const GamePad::ControlType control, const int player,
                         const float value, const bool pressed) override;

    void sendEvent (const GamePadEvent&);
};

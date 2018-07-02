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

#include "GamePad.h"

GamePadManager::GamePadManager() { }
GamePadManager::~GamePadManager()
{
    for (auto* source : sources)
    {
        source->removeListener (this);
        for (auto* pad : source->pads)
            pad->removeListener (this);
    }
    
    sources.clear (true);
}

void GamePadManager::addSource (GamePadSource* source)
{
    if (sources.contains (source))
        return;
    sources.add (source);
    source->addListener (this);
    for (auto* pad : source->pads)
        gamepadConnected (pad);
}

void GamePadManager::addDefaultSources() { }

void GamePadManager::sendEvent (const GamePadEvent& event)
{
    for (auto* listener : listeners)
        listener->handleGamePadEvent (event);
}

void GamePadManager::gamepadConnected (GamePad* pad)
{
    DBG("[emu] gamepad connected: " << pad->getName());
    pad->addListener (this);
}

void GamePadManager::gamepadDisconnected (GamePad* pad)
{
    DBG("[emu] gamepad disconnected: " << pad->getName());
    pad->removeListener (this);
}

void GamePadManager::buttonPressed (GamePad* pad, const GamePad::ControlType button)
{
    GamePadEvent event;
    event.type      = GamePadEvent::ButtonPress;
    event.control   = button;
    event.player    = pad->getPlayer();
    event.value     = 1.f;
    event.pressed   = true;

    sendEvent (event);
}

void GamePadManager::buttonReleased (GamePad* pad, const GamePad::ControlType button)
{
    GamePadEvent event;
    event.type      = GamePadEvent::ButtonRelease;
    event.control   = button;
    event.player    = pad->getPlayer();
    event.value     = 0.f;
    event.pressed   = false;

    sendEvent (event);
}

void GamePadManager::controlChanged (GamePad* pad, const GamePad::ControlType control, const int player, 
                                     const float value, const bool pressed)
{
    // DBG("control: " << pad->getName() << ": " << player << " - " << value << " - " << (int) pressed);

    GamePadEvent event;
    event.type      = GamePadEvent::ValueChange;
    event.control   = control;
    event.player    = player;
    event.value     = value;
    event.pressed   = pressed;

    sendEvent (event);
}

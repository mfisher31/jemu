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

#include "MainComponent.h"

MainComponent::MainComponent()
{
    image = Image (Image::RGB, 256, 240, true);
    setSize (image.getWidth(), image.getHeight());
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    if (image.isNull() || !image.isValid())
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        g.setFont (Font (16.0f));
        g.setColour (Colours::white);
        g.drawText ("Nestopia", 0, 0, getWidth(), getHeight(), Justification::centred, false);
    }
    else
    {
        // g.fillAll (Colours::black);
        // g.drawImage (image, 0, 0, getWidth(), getHeight(), 0, 0, image.getWidth(), image.getHeight());
        // g.drawImage (image, getLocalBounds().toFloat(), RectanglePlacement::fillDestination);
        g.drawImageWithin (image, 2, 2, getWidth() - 4, getHeight() - 4,
                           RectanglePlacement::centred, false);
    }
}

void MainComponent::resized()
{
}

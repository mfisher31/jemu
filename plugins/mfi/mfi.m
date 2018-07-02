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

#import <GameController/GameController.h>
#include <jemu/plugin.h>

@interface MfiGamepadDiscovery : NSObject
@end

@implementation MfiGamepadDiscovery
JemuGamePad pad;
- (id) init
{
    self = [super init];
    return self;
}

- (void) dealloc
{
    [self stopDiscovery];
    [super dealloc];
}

- (void) discoverControllers
{
    [GCController startWirelessControllerDiscoveryWithCompletionHandler: nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(controllerAdded:)
                                          name:GCControllerDidConnectNotification
                                          object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                          selector:@selector(controllerRemoved:)
                                          name:GCControllerDidDisconnectNotification
                                          object:nil];
}

- (void) stopDiscovery
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                          name:GCControllerDidConnectNotification
                                          object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                          name:GCControllerDidDisconnectNotification
                                          object:nil];
    [GCController stopWirelessControllerDiscovery];
}

- (void) controllerAdded:(NSNotification*)notification
{
    GCController* controller = (GCController*) [notification object];
    // self.owner->connect (controller);
}

- (void) controllerRemoved:(NSNotification*) notification
{
    GCController* controller = (GCController*) [notification object];
    // self.owner->disconnect (controller);
}

- (void) setupExtended:(GCController*) controller
{
    controller.extendedGamepad.dpad.up.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.dpad.down.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.dpad.left.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.dpad.right.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.buttonA.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.buttonB.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.buttonX.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
        if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.buttonY.pressedChangedHandler =
        ^(GCControllerButtonInput* button, float value, BOOL pressed)
    {
       if (pressed) {
            // sendButtonPress (Gamepad::UP);
        } else { 
            // sendButtonRelease (Gamepad::UP);
        }
    };

    controller.extendedGamepad.valueChangedHandler = 
        ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
    {
        NSString* message = @"";
        const bool doDebug = false;

        // L2
        if (gamepad.leftTrigger == element && gamepad.leftTrigger.isPressed) {
        }
        // L2
        if (gamepad.rightTrigger == element && gamepad.rightTrigger.isPressed) {
        }
        // L1
        if (gamepad.leftShoulder == element && gamepad.leftShoulder.isPressed) {
        }
        // L2
        if (gamepad.rightShoulder == element && gamepad.rightShoulder.isPressed) {
        }
        // X
        if (gamepad.buttonX == element) {
            // sendControlChange (Gamepad::buttonX, gamepad.buttonX.value, gamepad.buttonX.isPressed);
        }

        if (gamepad.buttonY == element) {
            // sendControlChange (Gamepad::buttonY, gamepad.buttonY.value, gamepad.buttonY.isPressed);
        }

        if (gamepad.buttonA == element) {
            // sendControlChange (Gamepad::buttonA, gamepad.buttonA.value, gamepad.buttonA.isPressed);
        }

        if (gamepad.buttonB == element) {
            // sendControlChange (Gamepad::buttonB, gamepad.buttonB.value, gamepad.buttonB.isPressed);
        }

        // d-pad
        if (gamepad.dpad == element)
        {
            if (gamepad.dpad.up.isPressed) {
                message = @"dpad: Up";
            }
            if (gamepad.dpad.down.isPressed) {
                message = @"dpad: DOWN";
            }
            if (gamepad.dpad.left.isPressed) {
                message = @"dpad: LEFT";
            }
            if (gamepad.dpad.right.isPressed) {
                message = @"dpad: RIGHT";
            }
        }

        // left stick
        if (gamepad.leftThumbstick == element) {

            if (gamepad.leftThumbstick.up.isPressed) {
                message = [NSString stringWithFormat:@"Left Stick %f", gamepad.leftThumbstick.yAxis.value];
            }

            if (gamepad.leftThumbstick.down.isPressed) {
                message = [NSString stringWithFormat:@"Left Stick %f", gamepad.leftThumbstick.yAxis.value];
            }

            if (gamepad.leftThumbstick.left.isPressed) {
                message = [NSString stringWithFormat:@"Left Stick %f", gamepad.leftThumbstick.xAxis.value];
            }

            if (gamepad.leftThumbstick.right.isPressed) {
                message = [NSString stringWithFormat:@"Left Stick %f", gamepad.leftThumbstick.xAxis.value];
            }
        }

        if (doDebug) {
            // NSLog("[EMU] gamepad: " << nsStringToJuce (message));
        }
    };
}

- (void) setupStandard:(GCController*) controller
{

}

@end

static void jemu_mfi_discover (JemuHandle handle, bool scan);
static const void* jemu_mfi_extension (const char* identifier);

static JemuHandle jemu_mfi_instantiate (const char* bundle_path)
{
    MfiGamepadDiscovery* instance = [[MfiGamepadDiscovery alloc] init];
    [instance retain];
    return (JemuHandle) instance;
}

static void jemu_mfi_destroy (JemuHandle handle) {
    MfiGamepadDiscovery* instance = (MfiGamepadDiscovery*) handle;
    [instance release];
}

static void jemu_mfi_discover (JemuHandle handle, bool scan)
{
    MfiGamepadDiscovery* instance = (MfiGamepadDiscovery*) handle;
    if (scan) {
        [instance discoverControllers];
    } else {
        [instance stopDiscovery];
    }
}

static JemuDescriptor _mfi_descriptor = {
    .ID             = JEMU_MFI,
    .instantiate    = &jemu_mfi_instantiate,
    .destroy        = &jemu_mfi_destroy,
    .extension      = &jemu_mfi_extension
};

static JemuGamePad _mfi_gamepad = {
    .discover       = &jemu_mfi_discover,
    .name           = NULL
};

static const void* jemu_mfi_extension (const char* identifier) {
    return strcmp (identifier, JEMU_GAME_PAD) == 0
        ? (const void*) &_mfi_gamepad : NULL;
}

JEMU_SYMBOL_EXPORT
const JemuDescriptor* jemu_descriptor (const uint32_t index)
{
    return (index == 0) ? &_mfi_descriptor : NULL;
}

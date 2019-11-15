#import "iNDSMFIControllerSupport.h"

#include "emu.h"

@implementation iNDSMFIControllerSupport {
    GCController *_controller;
    
    NSDictionary *_controllerElementToButtonIdMapping;
}

+(id) instance {
    static iNDSMFIControllerSupport *instance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [iNDSMFIControllerSupport new];
    });
    
    return instance;
}

// To my knowledge
-(BOOL) canSupportGamepads {
    return [GCController class] != nil && ![[NSUserDefaults standardUserDefaults] boolForKey:@"disableGamepad"];
}

-(void) startMonitoringGamePad {
    if (![self canSupportGamepads])
        return;
    
    // We need to start discovering controllers. This is best done using NSNotificationCenter and the GCController APIs.
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onControllerConnected:) name:GCControllerDidConnectNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onControllerDisconnected:) name:GCControllerDidDisconnectNotification object:nil];
    
    NSLog(@"Starting controller discovery...");
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:nil];
    
    if ([[GCController controllers] count] > 0) {
        // We already have a connected controller, jump directly to our connected method.
        [self onControllerConnected:nil];
    }
}

-(void) onControllerConnected:(NSNotification *)notification {
    // If we already have a controller, we shouldn't be using this new one.
    if (_controller != nil) return;
    
    if (notification) {
        _controller = notification.object;
    } else {
        _controller = [[GCController controllers] firstObject];
    }
    NSLog(@"Connected controller %@", _controller);
    
    // Basic control mapping.
    // TODO: Allow users to modify this.
    _controllerElementToButtonIdMapping = @{
        [NSValue valueWithNonretainedObject:_controller.gamepad.dpad.right] : @(BUTTON_RIGHT),
        [NSValue valueWithNonretainedObject:_controller.gamepad.dpad.left] : @(BUTTON_LEFT),
        [NSValue valueWithNonretainedObject:_controller.gamepad.dpad.down] : @(BUTTON_DOWN),
        [NSValue valueWithNonretainedObject:_controller.gamepad.dpad.up] : @(BUTTON_UP),
        // [NSValue valueWithNonretainedObject:_controller.gamepad.NONE] : @(BUTTON_SELECT),
        // [NSValue valueWithNonretainedObject:_controller.gamepad.NONE] : @(BUTTON_START),
        [NSValue valueWithNonretainedObject:_controller.gamepad.buttonB] : @(BUTTON_B),
        [NSValue valueWithNonretainedObject:_controller.gamepad.buttonA] : @(BUTTON_A),
        [NSValue valueWithNonretainedObject:_controller.gamepad.buttonY] : @(BUTTON_Y),
        [NSValue valueWithNonretainedObject:_controller.gamepad.buttonX] : @(BUTTON_X),
        [NSValue valueWithNonretainedObject:_controller.gamepad.leftShoulder] : @(BUTTON_L),
        [NSValue valueWithNonretainedObject:_controller.gamepad.rightShoulder] : @(BUTTON_R),
    };
    
    // To prevent retain cycles.
    __weak id weakSelf = self;
    
    // Next, setup the mapping for each of our buttons on the controller.
    // We will simply forward this on to our helper method for telling the emulator that we actually changed a button.
    for (NSValue *value in [_controllerElementToButtonIdMapping allKeys]) {
        GCControllerButtonInput *button = [value nonretainedObjectValue];
        
        button.valueChangedHandler = ^(GCControllerButtonInput *button, float value, BOOL pressed) {
            [weakSelf onGamepad:_controller.gamepad buttonChanged:button];
        };
    }
    
    _controller.controllerPausedHandler = ^(GCController * _Nonnull controller) {
        EMU_buttonDown(BUTTON_START);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 0.18), dispatch_get_main_queue(), ^(void){
            EMU_buttonUp(BUTTON_START);
        });
    };
    
    // Make sure the screen doesn't turn off on us while we're playing
    dispatch_async(dispatch_get_main_queue(), ^{
        [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    });
}

-(void) onControllerDisconnected:(NSNotification *)notification {
    NSLog(@"Controller disconnected: %@", [_controller vendorName]);
    
    if (notification.object == _controller) {
        _controller = nil;
        _controllerElementToButtonIdMapping = nil;
        
        if ([GCController controllers].count > 0) {
            // Connect the next controller in-line
            [self onControllerConnected:nil];
        }
    }
}

-(void) onGamepad:(GCGamepad *) gamepad buttonChanged:(GCControllerButtonInput *) button {
    NSValue *asValue = [NSValue valueWithNonretainedObject:button];
    BUTTON_ID ndsButton = (BUTTON_ID) [_controllerElementToButtonIdMapping[asValue] intValue];
        
    if (button.isPressed) {
        EMU_buttonDown(ndsButton);
    } else {
        EMU_buttonUp(ndsButton);
    }
}

-(void) stopMonitoringGamePad {
    if (![self canSupportGamepads])
        return;
    
    // Stop this if we were doing it already
    [GCController stopWirelessControllerDiscovery];
    
    _controller = nil;
    _controllerElementToButtonIdMapping = nil;
    
    // Allow the screen to turn off again.
    dispatch_async(dispatch_get_main_queue(), ^{
        [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    });
}

@end

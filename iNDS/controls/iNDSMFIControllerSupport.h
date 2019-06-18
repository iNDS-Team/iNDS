#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

// This class will manage MFi (Made For iPhone) controllers.
// It will only support the DS buttons, not the touch screen.
// It will use the standard gamepad (non-extended) for the most compatiblilty.
// It should still work on iOS <7, but I have no way of testing that.
@interface iNDSMFIControllerSupport : NSObject

// The instance of this singleton
+(instancetype) instance;

-(void) startMonitoringGamePad;
-(void) stopMonitoringGamePad;
- (void)listenforPause:(void(^)(GCController *controller))handler;

@end

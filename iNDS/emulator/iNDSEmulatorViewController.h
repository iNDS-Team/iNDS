//
//  iNDSEmulatorViewController.h
//  iNDS
//
//  Created by iNDS on 6/11/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "iNDSGame.h"

@class iNDSEmulationProfile;
@interface iNDSEmulatorViewController : UIViewController <UIAlertViewDelegate>

@property (strong, nonatomic) iNDSGame *game;
@property (strong, nonatomic) iNDSEmulationProfile * profile;
@property (copy, nonatomic)   NSString *saveState;
@property (assign, nonatomic) CGFloat speed;

@property (weak, nonatomic) IBOutlet UIView *gameContainer;
@property (weak, nonatomic) IBOutlet UIView *settingsContainer;
@property (weak, nonatomic) IBOutlet UIView *darkenView;

- (void)pauseEmulation;
- (void)resumeEmulation;
- (void)saveStateWithName:(NSString*)saveStateName;
- (void)changeGame:(iNDSGame *)newGame;
- (void)enterEditMode;
- (void)exitEditMode;
- (IBAction)toggleSettings:(id)sender;
- (void) setSettingsHeight:(CGFloat) height;
- (void)loadProfile:(iNDSEmulationProfile *)profile;
- (void)newSaveState;
- (void)reloadEmulator;
- (void)setLidClosed:(BOOL)closed;
- (void) userRequestedToPlayROM:(NSNotification *) notification;
@end

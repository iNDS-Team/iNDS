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
@property (copy, nonatomic) NSString *saveState;
@property (assign, nonatomic) NSInteger speed;

- (void)pauseEmulation;
- (void)resumeEmulation;
- (void)saveStateWithName:(NSString*)saveStateName;
- (void)changeGame;
@end

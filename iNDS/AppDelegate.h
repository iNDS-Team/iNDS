//
//  AppDelegate.h
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "iNDSEmulatorViewController.h"

#import "iNDSGame.h"

#define kBugUrl @"http://69.167.218.245:6768/iNDS/bugreport"
//#define kBugUrl @"http://www.williamlcobb.com/iNDS/bugreport"

@class SCLAlertView;
@class WCEasySettingsViewController;
@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) iNDSGame *currentGame;
@property (strong, nonatomic) iNDSEmulatorViewController *currentEmulatorViewController;
@property (strong, nonatomic) NSURL *lastUrl;
@property (strong, nonatomic, getter=getSettingsViewController) WCEasySettingsViewController *settingsViewController;


+ (AppDelegate *)sharedInstance;

- (NSString *)cheatsDir;
- (NSString *)batteryDir;
- (NSString *)documentsPath;
- (NSString *)rootDocumentsPath;

- (void)showError:(NSString *)error;
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;
- (void)startGame:(iNDSGame *)game withSavedState:(NSInteger)savedState;
- (void)startBackgroundProcesses;
- (void)authDropbox;
- (void)downloadDB:(void(^)(int))handler;

- (BOOL)isSystemApplication;

extern NSString * const iNDSUserRequestedToPlayROMNotification;

@end



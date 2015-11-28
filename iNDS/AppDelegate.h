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

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (strong, nonatomic) iNDSGame *currentGame;
@property (strong, nonatomic) iNDSEmulatorViewController *currentEmulatorViewController;
@property (strong, nonatomic) NSURL *lastUrl;

+ (AppDelegate *)sharedInstance;

- (NSString *)batteryDir;
- (NSString *)documentsPath;

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;
- (void)startGame:(iNDSGame *)game withSavedState:(NSInteger)savedState;

@end
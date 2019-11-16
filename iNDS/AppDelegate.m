//
//  AppDelegate.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import <ObjectiveDropboxOfficial/ObjectiveDropboxOfficial.h>
#import "CHBgDropboxSync.h"
#import "SSZipArchive.h"
#import "LZMAExtractor.h"
#import "ZAActivityBar.h"
#import <SDImageCacheConfig.h>
#import <SDImageCache.h>

#import "iNDSDBManager.h"

#include <libkern/OSAtomic.h>
#include <execinfo.h>

#import "iNDSInitialViewController.h"
#import "SCLAlertView.h"

#import "AFHTTPSessionManager.h"

#import "WCEasySettingsViewController.h"

#import "iNDSSpeedTest.h"
#import "iNDSDropboxTableViewController.h"
#import "WCBuildStoreClient.h"
#import "iNDSBuildStoreTableViewController.h"

#import <UnrarKit/UnrarKit.h>

#import <objc/runtime.h>

@interface AppDelegate () {
    BOOL    backgroundProcessesStarted;
}

@end

NSString * const iNDSUserRequestedToPlayROMNotification = @"iNDSUserRequestedToPlayROMNotification";

@implementation AppDelegate

+ (AppDelegate*)sharedInstance
{
    return (AppDelegate *) [[UIApplication sharedApplication] delegate];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Defaults" ofType:@"plist"]]];
    
    //Create documents and battery folder if needed
    if (![[NSFileManager defaultManager] fileExistsAtPath:self.documentsPath]) {
        [[NSFileManager defaultManager] createDirectoryAtPath:self.documentsPath withIntermediateDirectories:YES attributes:nil error:nil];
    }
    if (![[NSFileManager defaultManager] fileExistsAtPath:self.batteryDir]) {
        [[NSFileManager defaultManager] createDirectoryAtPath:self.batteryDir withIntermediateDirectories:YES attributes:nil error:nil];
    }
    
    [self.window setTintColor:[UIColor colorWithRed:1 green:59/255.0 blue:48/255.0 alpha:1]];
    
    [[UIBarButtonItem appearance] setBackButtonTitlePositionAdjustment:UIOffsetMake(-200, 0)
                                                         forBarMetrics:UIBarMetricsDefault];
    
    [self setupSDWebImageCache];
    
    [self authDropbox];
    
    return YES;
}

- (void)setupSDWebImageCache {
    [SDImageCache sharedImageCache].config.maxCacheAge = 60 * 60 * 24 * 7;
}

- (void)authDropbox {
    NSString* errorMsg = nil;
    if ([[self appKey] rangeOfCharacterFromSet:[[NSCharacterSet alphanumericCharacterSet] invertedSet]].location != NSNotFound) {
        errorMsg = @"You must set the App Key correctly for Dropbox to work!";
    } else if ([[self appSecret] rangeOfCharacterFromSet:[[NSCharacterSet alphanumericCharacterSet] invertedSet]].location != NSNotFound) {
        errorMsg = @"You must set the App Secret correctly for Dropbox to work!";
    } else {
        NSString *plistPath = [[NSBundle mainBundle] pathForResource:@"Info" ofType:@"plist"];
        NSData *plistData = [NSData dataWithContentsOfFile:plistPath];
        NSDictionary *loadedPlist =
        [NSPropertyListSerialization propertyListFromData:plistData mutabilityOption:0 format:NULL errorDescription:NULL];
        NSString *scheme = [[[[loadedPlist objectForKey:@"CFBundleURLTypes"] objectAtIndex:0] objectForKey:@"CFBundleURLSchemes"] objectAtIndex:0];
        if ([scheme isEqual:@"db-APP_KEY"]) {
            errorMsg = @"You must set the URL Scheme correctly in iNDS-Info.plist for Dropbox to work!";
        }
    }
    
    if (errorMsg != nil) {
        NSLog(@"Error: %@", errorMsg);
    } else {
        
        BOOL willPerformMigration = [DBClientsManager checkAndPerformV1TokenMigration:^(BOOL shouldRetry, BOOL invalidAppKeyOrSecret,
                                                                                        NSArray<NSArray<NSString *> *> *unsuccessfullyMigratedTokenData) {
            
            if (shouldRetry) {
                // Store this BOOL somewhere to retry when network connection has returned
            }
            
            if ([unsuccessfullyMigratedTokenData count] != 0) {
                NSLog(@"The following tokens were unsucessfully migrated:");
                for (NSArray<NSString *> *tokenData in unsuccessfullyMigratedTokenData) {
                    NSLog(@"DropboxUserID: %@, AccessToken: %@, AccessTokenSecret: %@, StoredAppKey: %@", tokenData[0],
                          tokenData[1], tokenData[2], tokenData[3]);
                }
            }
            
            if (!invalidAppKeyOrSecret && !shouldRetry && [unsuccessfullyMigratedTokenData count] == 0) {
                DBTransportDefaultConfig *transportConfiguration = [[DBTransportDefaultConfig alloc] initWithAppKey:[self appKey] forceForegroundSession:YES];
                [DBClientsManager setupWithTransportConfig:transportConfiguration];
                [CHBgDropboxSync start];
            }
        } queue:nil appKey:[self appKey] appSecret:[self appSecret]];
        
        if (!willPerformMigration) {
            DBTransportDefaultConfig *transportConfiguration = [[DBTransportDefaultConfig alloc] initWithAppKey:[self appKey] forceForegroundSession:YES];
            [DBClientsManager setupWithTransportConfig:transportConfiguration];
            [CHBgDropboxSync start];
        }
    }
}

- (void)startBackgroundProcesses
{
    if (backgroundProcessesStarted) {
        return;
    }
    backgroundProcessesStarted = YES;
    /*
    dispatch_async(dispatch_get_main_queue(), ^{
        //Show Twitter alert
        if (![[NSUserDefaults standardUserDefaults] objectForKey:@"TwitterAlert"]) {
            [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"TwitterAlert"];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] < 5) {
            [[NSUserDefaults standardUserDefaults] setInteger: [[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] + 1 forKey:@"TwitterAlert"];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] == 5) {
            [[NSUserDefaults standardUserDefaults] setInteger:10 forKey:@"TwitterAlert"];
            SCLAlertView * alert = [[SCLAlertView alloc] init];
            alert.iconTintColor = [UIColor whiteColor];
            alert.shouldDismissOnTapOutside = YES;
            [alert addButton:@"Follow" actionBlock:^(void) {
                [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/miniroo321"]];
            }];
            UIImage * twitterImage = [[UIImage imageNamed:@"Twitter.png"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
            [alert showCustom:[self topMostController] image:twitterImage color:[UIColor colorWithRed:85/255.0 green:175/255.0 blue:238/255.0 alpha:1] title:@"Love iNDS?" subTitle:@"Show some love and get updates about the newest emulators by following the developer on Twitter!" closeButtonTitle:@"No, Thanks" duration:0.0];
        }
    });
     */
}


- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    NSLog(@"Opening: %@", url);
    if ([[url scheme] hasPrefix:@"db"]) {
        NSLog(@"DB");
        //        if ([[DBSession sharedSession] handleOpenURL:url]) {
        //            if ([[DBSession sharedSession] isLinked]) {
        DBOAuthResult *authResult = [DBClientsManager handleRedirectURL:url];
        if (authResult != nil) {
            if ([authResult isSuccess]) {
                NSLog(@"Success! User is logged into Dropbox.");
                SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
                [alert showInfo:NSLocalizedString(@"SUCCESS", nil) subTitle:NSLocalizedString(@"SUCCESS_DETAIL", nil) closeButtonTitle:@"Okay!" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"enableDropbox"];
                [CHBgDropboxSync clearLastSyncData];
                [CHBgDropboxSync start];
            } else if ([authResult isCancel]) {
                NSLog(@"Authorization flow was manually canceled by user!");
            } else if ([authResult isError]) {
                NSLog(@"Error: %@", authResult);
            }
        }
        //            }
        return YES;
        //        }
    } else if ([[[url scheme] lowercaseString] isEqualToString:@"inds"]) {
        NSString *name = [[url host] stringByRemovingPercentEncoding];
        
        iNDSGame *rom = [iNDSGame gameWithName:name];
        
        if (rom) {
            NSLog(@"Found ROM");
            [[NSNotificationCenter defaultCenter] postNotificationName:iNDSUserRequestedToPlayROMNotification object:rom userInfo:nil];
            return YES;
        } else {
            NSLog(@"Could not find ROM");
        }
        
        return NO;
    } else if (url.isFileURL && [[NSFileManager defaultManager] fileExistsAtPath:url.path]) {
        NSLog(@"Zip File (maybe)");
        NSFileManager *fm = [NSFileManager defaultManager];
        NSError *err = nil;
        if ([url.pathExtension.lowercaseString isEqualToString:@"zip"] || [url.pathExtension.lowercaseString isEqualToString:@"7z"] || [url.pathExtension.lowercaseString isEqualToString:@"rar"]) {
            
            // expand zip
            // create directory to expand
            NSString *dstDir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"extract"];
            if ([fm createDirectoryAtPath:dstDir withIntermediateDirectories:YES attributes:nil error:&err] == NO) {
                NSLog(@"Could not create directory to expand zip: %@ %@", dstDir, err);
                [fm removeItemAtURL:url error:NULL];
                [self showError:@"Unable to extract archive file."];
                [fm removeItemAtPath:url.path error:NULL];
                return NO;
            }
            
            // expand
            NSLog(@"Expanding: %@", url.path);
            NSLog(@"To: %@", dstDir);
            if ([url.pathExtension.lowercaseString isEqualToString:@"zip"]) {
                [SSZipArchive unzipFileAtPath:url.path toDestination:dstDir];
            } else if ([url.pathExtension.lowercaseString isEqualToString:@"7z"]) {
                if (![LZMAExtractor extract7zArchive:url.path tmpDirName:@"extract"]) {
                    NSLog(@"Unable to extract 7z");
                    [self showError:@"Unable to extract .7z file."];
                    [fm removeItemAtPath:url.path error:NULL];
                    return NO;
                }
            } else { //Rar
                NSError *archiveError = nil;
                URKArchive *archive = [[URKArchive alloc] initWithPath:url.path error:&archiveError];
                if (!archive) {
                    NSLog(@"Unable to open rar: %@", archiveError);
                    [self showError:@"Unable to read .rar file."];
                    return NO;
                }
                //Extract
                NSError *error;
                [archive extractFilesTo:dstDir overwrite:YES progress:nil error:&error];
                if (error) {
                    NSLog(@"Unable to extract rar: %@", archiveError);
                    [self showError:@"Unable to extract .rar file."];
                    [fm removeItemAtPath:url.path error:NULL];
                    return NO;
                }
            }
            NSLog(@"Searching");
            NSMutableArray * foundItems = [NSMutableArray array];
            NSError *error;
            // move .iNDS to documents and .dsv to battery
            for (NSString *path in [fm subpathsAtPath:dstDir]) {
                if ([path.pathExtension.lowercaseString isEqualToString:@"nds"] && ![[path.lastPathComponent substringToIndex:1] isEqualToString:@"."]) {
                    NSLog(@"found ROM in zip: %@", path);
                    [fm moveItemAtPath:[dstDir stringByAppendingPathComponent:path] toPath:[self.documentsPath stringByAppendingPathComponent:path.lastPathComponent] error:&error];
                    [foundItems addObject:path.lastPathComponent];
                } else if ([path.pathExtension.lowercaseString isEqualToString:@"dsv"]) {
                    NSLog(@"found save in zip: %@", path);
                    [fm moveItemAtPath:[dstDir stringByAppendingPathComponent:path] toPath:[self.batteryDir stringByAppendingPathComponent:path.lastPathComponent] error:&error];
                    [foundItems addObject:path.lastPathComponent];
                } else if ([path.pathExtension.lowercaseString isEqualToString:@"dst"]) {
                    NSLog(@"found dst save in zip: %@", path);
                    NSString *newPath = [[path stringByDeletingPathExtension] stringByAppendingPathExtension:@"dsv"];
                    [fm moveItemAtPath:[dstDir stringByAppendingPathComponent:path] toPath:[self.batteryDir stringByAppendingPathComponent:newPath.lastPathComponent] error:&error];
                    [foundItems addObject:path.lastPathComponent];
                } else {
                    BOOL isDirectory;
                    if ([[NSFileManager defaultManager] fileExistsAtPath:[dstDir stringByAppendingPathComponent:path] isDirectory:&isDirectory]) {
                        if (!isDirectory) {
                            [[NSFileManager defaultManager] removeItemAtPath:[dstDir stringByAppendingPathComponent:path] error:NULL];
                        }
                    }
                }
                if (error) {
                    NSLog(@"Error searching archive: %@", error);
                }
            }
            if (foundItems.count == 0) {
                [self showError:@"No roms or saves found in archive. Make sure the zip contains a .nds file or a .dsv file"];
            }
            else if (foundItems.count == 1) {
                [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Added: %@", foundItems[0]] duration:3];
            } else {
                [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Added %ld items", (long)foundItems.count] duration:3];
            }
            // remove unzip dir
            [fm removeItemAtPath:dstDir error:NULL];
        } else if ([url.pathExtension.lowercaseString isEqualToString:@"nds"]) {
            // move to documents
            [ZAActivityBar showSuccessWithStatus:[NSString stringWithFormat:@"Added: %@", url.path.lastPathComponent] duration:3];
            [fm moveItemAtPath:url.path toPath:[self.documentsPath stringByAppendingPathComponent:url.lastPathComponent] error:&err];
        } else {
            NSLog(@"Invalid File!");
            NSLog(@"%@", url.pathExtension.lowercaseString);
            [fm removeItemAtPath:url.path error:NULL];
            [self showError:@"Unable to open file: Unknown extension"];
            
        }
        [fm removeItemAtPath:url.path error:NULL];
        return YES;
    } else {
        [self showError:[NSString stringWithFormat:@"Unable to open file: Unknown Error (%i, %i, %@)", url.isFileURL, [[NSFileManager defaultManager] fileExistsAtPath:url.path], url]];
        [[NSFileManager defaultManager] removeItemAtPath:url.path error:NULL];
        
    }
    return NO;
}

- (void)showError:(NSString *)error
{
    dispatch_async(dispatch_get_main_queue(), ^{
        SCLAlertView * alertView = [[SCLAlertView alloc] init];
        alertView.shouldDismissOnTapOutside = YES;
        [alertView showError:[self topMostController] title:@"Error!" subTitle:error closeButtonTitle:@"Okay" duration:0.0];
    });
}

- (UIViewController*) topMostController
{
    UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (topController.presentedViewController) {
        topController = topController.presentedViewController;
    }
    return topController;
}

- (NSString *)cheatsDir
{
    return [self batteryDir]; //Changed this because of dropbox sync troubles
}

- (NSString *)batteryDir
{
    return [self.documentsPath stringByAppendingPathComponent:@"Battery"];
}

- (NSString *)documentsPath
{
    if ([self isSystemApplication]) {
        return [[self rootDocumentsPath] stringByAppendingPathComponent:@"iNDS"];
    } else {
        return [self rootDocumentsPath];
    }
}

- (NSString *)rootDocumentsPath
{
    return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
}

- (NSString *)libraryPath
{
    return [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) objectAtIndex:0];
}

- (NSString *)appKey
{
    return [[NSUserDefaults standardUserDefaults] stringForKey:@"dbAppKey"];
}

- (NSString *)appSecret
{
    return [[NSUserDefaults standardUserDefaults] stringForKey:@"dbAppSecret"];
}

- (void)startGame:(iNDSGame *)game withSavedState:(NSInteger)savedState
{;
    if (!self.currentEmulatorViewController) {
        iNDSEmulatorViewController *emulatorViewController = (iNDSEmulatorViewController *)[[UIStoryboard storyboardWithName:@"MainStoryboard" bundle:nil] instantiateViewControllerWithIdentifier:@"emulatorView"];
        emulatorViewController.game = game;
        emulatorViewController.saveState = [game pathForSaveStateAtIndex:savedState];
        [AppDelegate sharedInstance].currentEmulatorViewController = emulatorViewController;
        iNDSInitialViewController *rootViewController = (iNDSInitialViewController*)[self topMostController];
        //[rootViewController doSlideIn:nil];
        [rootViewController presentViewController:emulatorViewController animated:YES completion:nil];
    } else {
        self.currentEmulatorViewController.saveState = [game pathForSaveStateAtIndex:savedState];
        [self.currentEmulatorViewController changeGame:game];
    }
}

- (void)moveFolderAtPath:(NSString *)oldDirectory toPath:(NSString *)newDirectory
{
    NSLog(@"Moving %@ to %@", oldDirectory, newDirectory);
    NSFileManager *fm = [NSFileManager defaultManager];
    NSError *error;
    if (![fm fileExistsAtPath:newDirectory]) {
        [fm createDirectoryAtPath:newDirectory withIntermediateDirectories:NO attributes:nil error:&error];
        if (error) {
            NSLog(@"%@", error);
        }
    }
    NSArray *files = [fm contentsOfDirectoryAtPath:oldDirectory error:&error];
    if (error) NSLog(@"%@", error);
    for (NSString *file in files) {
        BOOL isDirectory;
        if ([fm fileExistsAtPath:[oldDirectory stringByAppendingPathComponent:file] isDirectory:&isDirectory] && isDirectory) {
            [self moveFolderAtPath:[oldDirectory stringByAppendingPathComponent:file] toPath:[newDirectory stringByAppendingPathComponent:file]];
        } else {
            [fm moveItemAtPath:[oldDirectory stringByAppendingPathComponent:file]
                        toPath:[newDirectory stringByAppendingPathComponent:file]
                         error:&error];
            if (error) NSLog(@"%@", error);
        }
    }
    if (!error) {
        [fm removeItemAtPath:oldDirectory error:nil];
    }
}

-(BOOL)isSystemApplication {
    return [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] pathComponents].count == 4;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}



- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Send saved bug reports
    NSString *savePath = [AppDelegate.sharedInstance.documentsPath stringByAppendingPathComponent:@"bug.json"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:savePath]) {
        NSLog(@"Sending saved bug");
        NSMutableDictionary * parameters = [NSMutableDictionary dictionaryWithContentsOfFile:savePath];
        
        AFHTTPSessionManager *manager = [[AFHTTPSessionManager alloc]initWithSessionConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];
        manager.requestSerializer = [AFJSONRequestSerializer serializer];
        [manager.requestSerializer setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
        manager.responseSerializer.acceptableContentTypes = [NSSet setWithObject:@"text/html"];
        
        [manager POST:kBugUrl parameters:parameters progress:nil success:^(NSURLSessionDataTask * _Nonnull task, id  _Nullable responseObject) {
            [[NSFileManager defaultManager] removeItemAtPath:savePath error:nil];
            [ZAActivityBar showSuccessWithStatus:@"Sent a saved bug report" duration:5];
        } failure:^(NSURLSessionDataTask * _Nullable task, NSError * _Nonnull error) {
            
        }];
    }
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void)downloadDB:(void(^)(int))handler {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *stringURL = @"https://inds.nerd.net/editor/openvgdb.sqlite";
        NSURL  *url = [NSURL URLWithString:stringURL];
        NSData *urlData = [NSData dataWithContentsOfURL:url];
        if ( urlData )
        {
            NSString *dbPath = [[NSBundle mainBundle] pathForResource:@"openvgdb" ofType:@"sqlite"];
            
            [urlData writeToFile:dbPath atomically:YES];
            handler(0);
        }
        handler(1);
    });
}

- (void)checkForUpdate:(void(^)(int, NSString*))handler {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *stringURL = @"https://api.github.com/repos/iNDS-Team/iNDS/releases/latest";
        NSURL  *url = [NSURL URLWithString:stringURL];
        NSData *urlData = [NSData dataWithContentsOfURL:url];
        if ( urlData )
        {
            NSError *error = nil;
            id object = [NSJSONSerialization
                         JSONObjectWithData:urlData
                         options:0
                         error:&error];
            
            if(error) { /* JSON was malformed, act appropriately here */ }
            
            // the originating poster wants to deal with dictionaries;
            // assuming you do too then something like this is the first
            // validation step:
            if([object isKindOfClass:[NSDictionary class]])
            {
                NSDictionary *results = object;
                handler(0, results[@"tab_name"]);
                return;
            }
            handler(0, nil);
            return;
        }
        handler(1, nil);
    });
}

- (WCEasySettingsViewController *)getSettingsViewController
{
    
    if (!_settingsViewController) {
        _settingsViewController = [[WCEasySettingsViewController alloc] initWithStyle:UITableViewStyleGrouped];
        
        //Controls
        WCEasySettingsSection *controlsSection = [[WCEasySettingsSection alloc] initWithTitle:@"CONTROLS" subTitle:@""];
        controlsSection.items = @[[[WCEasySettingsSegment alloc] initWithIdentifier:@"controlPadStyle"
                                                                              title:@"Control Pad Style"
                                                                              items:@[
                                                                                      @"D-Pad",
                                                                                      @"Joystick"]],
                                  [[WCEasySettingsSlider alloc] initWithIdentifier:@"controlOpacity"
                                                                             title:@"Controller Opacity"],
                                  [[WCEasySettingsSegment alloc] initWithIdentifier:@"vibrationStr"
                                                                              title:@"Vibration Strength"
                                                                              items:@[
                                                                                      @"Off",
                                                                                      @"Light",
                                                                                      @"Heavy"]],
                                  [[WCEasySettingsSwitch alloc] initWithIdentifier:@"hapticForVibration"
                                                                             title:@"Haptic for Vibration"],
                                  [[WCEasySettingsSwitch alloc] initWithIdentifier:@"volumeBumper"
                                                                             title:@"Volume Button Bumpers"],
                                  [[WCEasySettingsSwitch alloc] initWithIdentifier:@"disableTouchScreen"
                                                                             title:@"Disable Touchscreen"]
                                  
                                  ];
        
        // Video
        WCEasySettingsSection *graphicsSection = [[WCEasySettingsSection alloc] initWithTitle:@"Video" subTitle:@"Video Options"];
        WCEasySettingsOption *filterOptions = [[WCEasySettingsOption alloc] initWithIdentifier:@"videoFilter"
                                                                                         title:@"Video Filter"
                                                                                       options:@[@"None",
                                                                                                 @"EPX",
                                                                                                 @"Super Eagle",
                                                                                                 @"2xSaI",
                                                                                                 @"Super 2xSaI",
                                                                                                 @"BRZ 2x (Recommended)",
                                                                                                 @"Low Quality 2x",
                                                                                                 @"BRZ 3x",
                                                                                                 @"High Quality 2x",
                                                                                                 @"High Quality 4x",
                                                                                                 @"BRZ 4x"]
                                                                               optionSubtitles:nil
                                                                                      subtitle:@"Video filters make the picture sharper but can cause the emulator to run slower. Filters are ordered by lowest quality at the top to best at the bottom. If you're not sure, you can experiment or pick the highest quality that still makes games run at 60fps."];
        graphicsSection.items = @[filterOptions];
        //        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        //            NSArray *filters = @[@(NONE), @(EPX), @(SUPEREAGLE), @(_2XSAI), @(SUPER2XSAI), @(BRZ2x), @(LQ2X), @(BRZ3x), @(HQ2X), @(HQ4X), @(BRZ4x)];
        //            NSArray *filterTimes = [iNDSSpeedTest filterTimesForFilters:filters];
        //            CGFloat coreTime = [[NSUserDefaults standardUserDefaults] floatForKey:@"coreTime"];
        //            NSMutableArray *filterSubtitles = [NSMutableArray new];
        //            for (NSNumber *time in filterTimes) {
        //                NSLog(@"%f (%f + %f)", ([time floatValue] + coreTime), [time floatValue], coreTime);
        //                CGFloat estimatedFps = 1/([time floatValue] + coreTime);
        //                [filterSubtitles addObject:[NSString stringWithFormat:@"%d FPS", MAX(60, (int)estimatedFps)]];
        //            }
        //            //filterOptions.optionSubtitles = filterSubtitles;
        //        });
        
        // Audio
        WCEasySettingsSection *audioSection = [[WCEasySettingsSection alloc] initWithTitle:@"Audio" subTitle:@""];
        audioSection.items = @[[[WCEasySettingsSwitch alloc] initWithIdentifier:@"disableSound"
                                                                          title:@"Disable Sound"],
                               [[WCEasySettingsSwitch alloc] initWithIdentifier:@"allowExternalAudio"
                                                                          title:@"Allow External Audio"],
                               [[WCEasySettingsSwitch alloc] initWithIdentifier:@"synchSound"
                                                                          title:@"Synchronous Audio"],
                               [[WCEasySettingsSwitch alloc] initWithIdentifier:@"enableMic"
                                                                          title:@"Enable Mic"]];
        
        
        //Dropbox
        WCEasySettingsSection *dropboxSection = [[WCEasySettingsSection alloc] initWithTitle:@"DROPBOX" subTitle:NSLocalizedString(@"ENABLE_DROPBOX", nil)];
        
        
        
        WCEasySettingsCustom *dropBox = [[WCEasySettingsCustom alloc] initWithTitle:@"Dropbox" subtitle:nil viewController:[[iNDSDropboxTableViewController alloc] initWithStyle:UITableViewStyleGrouped]];
        
        dropboxSection.items = @[dropBox];
        
        // Buildstore
        
        WCEasySettingsSection *buildStoreSection = [[WCEasySettingsSection alloc] initWithTitle:@"Build Store" subTitle:@"Automatically update app through the Build Store"];
        
        WCEasySettingsCustom *buildStore = [[WCEasySettingsCustom alloc] initWithTitle:@"Build Store"
                                                                              subtitle:@""
                                                                        viewController:[[iNDSBuildStoreTableViewController alloc] initWithStyle:UITableViewStyleGrouped]];
        buildStoreSection.items = @[buildStore];
        
        
        // Core
        WCEasySettingsSection *coreSection = [[WCEasySettingsSection alloc] initWithTitle:@"Core" subTitle:@"Frame Skip with speed up emulation."];
        WCEasySettingsOption *engineOption;
        if (sizeof(void*) == 4) { //32bit
            engineOption = [[WCEasySettingsOption alloc] initWithIdentifier:@"cpuMode"
                                                                      title:@"Emulator Engine"
                                                                    options:@[@"Interpreter",
                                                                              @"JIT Recompiler (Beta)"
                                                                              ]
                                                            optionSubtitles:nil
                                                                   subtitle:@"Warning, JIT is still experimental and can slow down or even crash iNDS"];
        } else if (sizeof(void*) == 8) {
            engineOption = [[WCEasySettingsOption alloc] initWithIdentifier:@"cpuMode"
                                                                      title:@"Emulator Engine"
                                                                    options:@[@"Interpreter"]
                                                            optionSubtitles:nil
                                                                   subtitle:@"JIT is not yet available for your device."];
        }
        
        WCEasySettingsSwitch *adv_timing = [[WCEasySettingsSwitch alloc] initWithIdentifier:@"adv_timing" title:@"Enable Advanced Bus Timing"];
        
        WCEasySettingsSlider2 *depth = [[WCEasySettingsSlider2 alloc] initWithIdentifier:@"depth" title:@"Depth Comparison Threshold" max:500];
        
        coreSection.items = @[engineOption,
                              adv_timing,
                              depth,
                              [[WCEasySettingsSegment alloc] initWithIdentifier:@"frameSkip"
                                                                          title:@"Frame Skip"
                                                                          items:@[@"None",
                                                                                  @"1",
                                                                                  @"2",
                                                                                  @"3",
                                                                                  @"4"]]
                              ];
        
        
        // Auto Save
        WCEasySettingsSection *emulatorSection = [[WCEasySettingsSection alloc] initWithTitle:@"Auto Save" subTitle:@""];
        emulatorSection.items = @[[[WCEasySettingsSwitch alloc] initWithIdentifier:@"periodicSave"
                                                                             title:@"Auto Save"]];
        
        // UI
        WCEasySettingsSection *interfaceSection = [[WCEasySettingsSection alloc] initWithTitle:@"Interface" subTitle:@""];
        interfaceSection.items = @[[[WCEasySettingsSwitch alloc] initWithIdentifier:@"fullScreenSettings"
                                                                              title:@"Full Screen Settings"],
                                   [[WCEasySettingsSwitch alloc] initWithIdentifier:@"showFPS"
                                                                              title:@"Show FPS"]];
        WCEasySettingsSection *iconSection = [[WCEasySettingsSection alloc] initWithTitle:@"ICONS" subTitle:@"Download the Latest Icon Pack"];
        WCEasySettingsButton *iconButton = [[WCEasySettingsButton alloc] initWithTitle:@"Update Icons" subtitle:nil callback:^(bool finished) {
            [[iNDSDBManager sharedInstance] closeDB];
            UIAlertController *alert = [UIAlertController alertControllerWithTitle:nil message:@"Please wait..." preferredStyle:UIAlertControllerStyleAlert];
            UIActivityIndicatorView *loadingIndicator = [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(10, 5, 50, 50)];
            loadingIndicator.hidesWhenStopped = true;
            loadingIndicator.activityIndicatorViewStyle = UIActivityIndicatorViewStyleGray;
            [loadingIndicator startAnimating];
            [alert.view addSubview:loadingIndicator];
            
                [self->_settingsViewController presentViewController:alert animated:YES completion:nil];
                
                [self downloadDB:^(int result) {
                    [[NSOperationQueue mainQueue] addOperationWithBlock:^ {
                    [self->_settingsViewController dismissViewControllerAnimated:true completion:^{
                        if (result == 0) {
                            UIAlertController *success = [UIAlertController alertControllerWithTitle:@"Success" message:@"Icons updated successfully." preferredStyle:UIAlertControllerStyleAlert];
                            UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                                  handler:^(UIAlertAction * action) {}];
                            [success addAction:defaultAction];
                            [self->_settingsViewController presentViewController:success animated:YES completion:nil];
                        } else {
                            UIAlertController *fail = [UIAlertController alertControllerWithTitle:@"Failure" message:@"Icon update failed." preferredStyle:UIAlertControllerStyleAlert];
                            UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                                  handler:^(UIAlertAction * action) {}];
                            [fail addAction:defaultAction];
                            [self->_settingsViewController presentViewController:fail animated:YES completion:nil];
                        }
                        [[iNDSDBManager sharedInstance] openDB];
                    }];
                }];
            }];
        }];
        
        WCEasySettingsSection *resetSection = [[WCEasySettingsSection alloc] initWithTitle:@"RESET" subTitle:@"Erase All Content"];
        WCEasySettingsButton *resetButton = [[WCEasySettingsButton alloc] initWithTitle:@"Reset" subtitle:nil callback:^(bool finished) {
            
            // prompt the user before deleting all data
            UIAlertController *warningAlert = [UIAlertController alertControllerWithTitle:@"Warning" message:@"This will erase all content. Do you want to continue?" preferredStyle:UIAlertControllerStyleAlert];
            UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * action) {}];
            UIAlertAction *continueAction = [UIAlertAction actionWithTitle:@"Continue" style:UIAlertActionStyleDestructive handler:^(UIAlertAction * _Nonnull action) {
                
                NSArray *everything = @[[self batteryDir],
                                        [self documentsPath]];
                NSFileManager *fileMgr = [NSFileManager defaultManager];
                for (NSString *path in everything) {
                    NSArray *files = [fileMgr contentsOfDirectoryAtPath:path error:nil];
                    for (NSString *file in files) {
                        NSLog(@"Removing %@", [path stringByAppendingPathComponent:file]);
                        [fileMgr removeItemAtPath:[path stringByAppendingPathComponent:file] error:nil];
                    }
                }
                
                UIAlertController *doneAlert = [UIAlertController alertControllerWithTitle:@"Success" message:@"Content successfully reset!" preferredStyle:UIAlertControllerStyleAlert];
                UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                      handler:^(UIAlertAction * action) {}];
                
                [doneAlert addAction:defaultAction];
                
                [self->_settingsViewController presentViewController:doneAlert animated:YES completion:nil];
            }];
            
            [warningAlert addAction:cancelAction];
            [warningAlert addAction:continueAction];
            [self->_settingsViewController presentViewController:warningAlert animated:YES completion:nil];
            
        }];
        resetSection.items = @[
                               resetButton
                               ];
        iconSection.items = @[ iconButton ];
        
        
        // Credits
        NSString *myVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
        NSString *noRar = @"";
        WCEasySettingsButton *updateButton = [[WCEasySettingsButton alloc] initWithTitle:@"Check for Updates" subtitle:nil callback:^(bool finished) {
            UIAlertController *alert = [UIAlertController alertControllerWithTitle:nil message:@"Please wait..." preferredStyle:UIAlertControllerStyleAlert];
            UIActivityIndicatorView *loadingIndicator = [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(10, 5, 50, 50)];
            loadingIndicator.hidesWhenStopped = true;
            loadingIndicator.activityIndicatorViewStyle = UIActivityIndicatorViewStyleGray;
            [loadingIndicator startAnimating];
            [alert.view addSubview:loadingIndicator];
            
            [self->_settingsViewController presentViewController:alert animated:YES completion:nil];
            
            [self checkForUpdate:^(int result, NSString *version) {
                [[NSOperationQueue mainQueue] addOperationWithBlock:^ {
                    [self->_settingsViewController dismissViewControllerAnimated:true completion:^{
                        if (result == 0 || version == nil) {
                            if ([[version substringFromIndex:1] compare:myVersion options:NSNumericSearch] == NSOrderedDescending) {
                                // update needed
                                UIAlertController *update = [UIAlertController alertControllerWithTitle:@"Update Available" message:@"An update is available on the iNDS Github" preferredStyle:UIAlertControllerStyleAlert];
                                UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                                      handler:^(UIAlertAction * action) {}];
                                [update addAction:defaultAction];
                                [self->_settingsViewController presentViewController:update animated:YES completion:nil];
                            } else {
                                UIAlertController *no_update = [UIAlertController alertControllerWithTitle:@"Up To Date" message:@"Your iNDS is already up to date." preferredStyle:UIAlertControllerStyleAlert];
                                UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                                      handler:^(UIAlertAction * action) {}];
                                [no_update addAction:defaultAction];
                                [self->_settingsViewController presentViewController:no_update animated:YES completion:nil];
                            }
                            
                        } else {
                            UIAlertController *fail = [UIAlertController alertControllerWithTitle:@"Failure" message:@"Checking for update failed." preferredStyle:UIAlertControllerStyleAlert];
                            UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
                                                                                  handler:^(UIAlertAction * action) {}];
                            [fail addAction:defaultAction];
                            [self->_settingsViewController presentViewController:fail animated:YES completion:nil];
                        }
                    }];
                }];
            }];
        }];
        WCEasySettingsSection *creditsSection = [[WCEasySettingsSection alloc]
                                                 initWithTitle:@"Info"
                                                 subTitle:[NSString stringWithFormat:@"Version %@ %@", myVersion, noRar]];
        
        creditsSection.items = @[[[WCEasySettingsUrl alloc] initWithTitle:@"iNDS Team"
                                                                 subtitle:@"Developer"
                                                                      url:@"https://github.com/iNDS-Team"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"NDS4iOS Team"
                                                                 subtitle:@"Ported DeSmuME to iOS"
                                                                      url:nil],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"DeSmuME"
                                                                 subtitle:@"Emulation Core"
                                                                      url:@"http://www.desmume.org/"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"Wiki Creator"
                                                                 subtitle:@"Pmp174"
                                                                      url:@"https://twitter.com/Pmp174"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"Source"
                                                                 subtitle:@"Github"
                                                                      url:@"https://github.com/iNDS-Team/iNDS"],
                                 updateButton];
        
        
        
        
        _settingsViewController.sections = @[controlsSection, dropboxSection, /*buildStoreSection,*/ graphicsSection, coreSection, emulatorSection, audioSection, interfaceSection, resetSection, iconSection, creditsSection];
    }
    
    return _settingsViewController;
}

@end

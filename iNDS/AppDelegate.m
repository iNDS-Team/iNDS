//
//  AppDelegate.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import <DropboxSDK/DropboxSDK.h>
#import "CHBgDropboxSync.h"
#import "SSZipArchive.h"
#import "LZMAExtractor.h"
#import "ZAActivityBar.h"

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

#ifdef UseRarKit
#import <UnrarKit/UnrarKit.h>
#import <dlfcn.h>
#endif

#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)

#import <objc/runtime.h>

@interface AppDelegate () {
    BOOL    backgroundProcessesStarted;
}

@end

@implementation AppDelegate

+ (AppDelegate*)sharedInstance
{
    return [[UIApplication sharedApplication] delegate];
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
    
    [[UIBarButtonItem appearance] setBackButtonTitlePositionAdjustment:UIOffsetMake(0, -60)
                                                         forBarMetrics:UIBarMetricsDefault];
    
    
    return YES;
}

- (void)startBackgroundProcesses
{
    if (backgroundProcessesStarted) {
        return;
    }
    backgroundProcessesStarted = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
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
            NSLog(@"%@", errorMsg);
        } else {
            DBSession* dbSession = [[DBSession alloc] initWithAppKey:[self appKey] appSecret:[self appSecret] root:kDBRootAppFolder];
            [DBSession setSharedSession:dbSession];
            [CHBgDropboxSync start];
        }
        
        
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
}


- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    NSLog(@"Opening: %@", url);
    if ([[[NSString stringWithFormat:@"%@", url] substringToIndex:2] isEqualToString: @"db"]) {
        NSLog(@"DB");
        if ([[DBSession sharedSession] handleOpenURL:url]) {
            if ([[DBSession sharedSession] isLinked]) {
                SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
                [alert showInfo:NSLocalizedString(@"SUCCESS", nil) subTitle:NSLocalizedString(@"SUCCESS_DETAIL", nil) closeButtonTitle:@"Okay!" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"enableDropbox"];
                [CHBgDropboxSync clearLastSyncData];
                [CHBgDropboxSync start];
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"enableDropbox"];
            }
            return YES;
        }
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
#ifdef UseRarKit
                // Hack to use csbypass to load rarkit
                if (SYSTEM_VERSION_GREATER_THAN(@"9.3.2")) {
                    [self showError:@"Rar support has been disabled due to code singing issues in this version of iOS."];
                    [fm removeItemAtPath:url.path error:NULL];
                    return NO;
                }
                
                // Csbypass
                NSString *unRarPath = [[NSBundle mainBundle] pathForResource:@"Frameworks/UnrarKit.framework/UnrarKit" ofType:nil];
                NSLog(@"UP: %@", unRarPath);
                void *handle = dlopen([unRarPath UTF8String], RTLD_NOW);
                if (!handle) {
                    [self showError:@"Unable to open Rarkit Framework. Please report."];
                    [fm removeItemAtPath:url.path error:NULL];
                    return NO;
                }
                
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
#else
                [self showError:@"Rar support has been disabled due to code singing issues. It will return in a future update."];
                [fm removeItemAtPath:url.path error:NULL];
                return NO;
#endif
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
                                  [[WCEasySettingsSwitch alloc] initWithIdentifier:@"vibrate"
                                                                             title:@"Vibration"],
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
                               [[WCEasySettingsSwitch alloc] initWithIdentifier:@"ignoreMute"
                                                                          title:@"Ignore Mute Button"],
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
                                                                    options:@[@"Interpretor"]
                                                            optionSubtitles:nil
                                                                   subtitle:@"JIT is not yet available for your device."];
        }
        
        coreSection.items = @[engineOption,
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
        
        
        // Credits
        NSString *myVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
        NSString *noRar = @"";
#ifndef UseRarKit
        noRar = @"(No Rar)";
#endif
        WCEasySettingsSection *creditsSection = [[WCEasySettingsSection alloc]
                                                 initWithTitle:@"Info"
                                                 subTitle:[NSString stringWithFormat:@"Version %@ %@", myVersion, noRar]];
        
        creditsSection.items = @[[[WCEasySettingsUrl alloc] initWithTitle:@"Will Cobb"
                                                                 subtitle:@"Developer"
                                                                      url:@"https://twitter.com/miniroo321"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"Twitter"
                                                                 subtitle:@"@iNDSapp"
                                                                      url:@"https://twitter.com/iNDSapp"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"NDS4iOS Team"
                                                                 subtitle:@"Ported DeSmuME to iOS"
                                                                      url:nil],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"DeSmuME"
                                                                 subtitle:@"Emulatiion Core"
                                                                      url:@"http://www.desmume.org/"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"Wiki Creator"
                                                                 subtitle:@"Pmp174"
                                                                      url:@"https://twitter.com/Pmp174"],
                                 [[WCEasySettingsUrl alloc] initWithTitle:@"Source"
                                                                 subtitle:@"Github"
                                                                      url:@"https://github.com/WilliamLCobb/iNDS"]];
                                 
        
        
        
        _settingsViewController.sections = @[controlsSection, dropboxSection, /*buildStoreSection,*/ graphicsSection, coreSection, emulatorSection, audioSection, interfaceSection, creditsSection];
    }
    
    return _settingsViewController;
}

@end

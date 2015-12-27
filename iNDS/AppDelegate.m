//
//  AppDelegate.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import <Fabric/Fabric.h>
#import <Crashlytics/Crashlytics.h>
#import <DropboxSDK/DropboxSDK.h>
#import "CHBgDropboxSync.h"
#import "SSZipArchive.h"
#import "LZMAExtractor.h"
#import "ZAActivityBar.h"
#import <UnrarKit/UnrarKit.h>

#include <libkern/OSAtomic.h>
#include <execinfo.h>

#import "iNDSInitialViewController.h"
#import "SCLAlertView.h"

#ifndef kCFCoreFoundationVersionNumber_iOS_7_0
#define kCFCoreFoundationVersionNumber_iOS_7_0 847.20
#endif

@implementation AppDelegate

+ (AppDelegate*)sharedInstance
{
    return [[UIApplication sharedApplication] delegate];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [Fabric with:@[[Crashlytics class]]];
    
    self.alertView = [[SCLAlertView alloc] init];
    self.alertView.shouldDismissOnTapOutside = YES;
    
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_7_0) {
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    } else {
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
    }
    
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Defaults" ofType:@"plist"]]];
    
    
    //Create iNDS folder and move old stuff
    if (![[NSFileManager defaultManager] fileExistsAtPath:self.documentsPath]) {
        NSError* error;
        [[NSFileManager defaultManager] createDirectoryAtPath:self.documentsPath withIntermediateDirectories:YES attributes:nil error:&error];
        if (error) {
            NSLog(@"Unable to create documents");
            [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"Unable to create iNDS folder in documents" closeButtonTitle:@"OK" duration:0.0];
        } else {
            //Move Battery
            if ([[NSFileManager defaultManager] fileExistsAtPath:self.oldBatteryDir]) {
                [[NSFileManager defaultManager] moveItemAtPath:self.oldBatteryDir toPath:self.batteryDir error:nil];
            }
            //Move Games
            NSDirectoryEnumerator* enumerator = [[NSFileManager defaultManager] enumeratorAtPath:self.oldDocumentsPath];
            NSString* file;
            while (file = [enumerator nextObject])
            {
                // check if it's a directory
                BOOL isDirectory = NO;
                NSString* fullPath = [self.oldDocumentsPath stringByAppendingPathComponent:file];
                [[NSFileManager defaultManager] fileExistsAtPath:fullPath
                                                 isDirectory: &isDirectory];
                if (!isDirectory)
                {
                    if ([fullPath.pathExtension isEqualToString:@"nds"]) {
                        //NSLog(@"Moving %@", fullPath);
                        //NSLog(@"TO: %@", [self.documentsPath stringByAppendingPathComponent:file]);
                        [[NSFileManager defaultManager] moveItemAtPath:fullPath toPath:[self.documentsPath stringByAppendingPathComponent:file] error:&error];
                        if (error) {
                            NSLog(@"E: %@", error);
                        }
                    }
                    
                }
            }
        }
    }
    
    //Dropbox DBSession Auth
    
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
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        DBSession* dbSession = [[DBSession alloc] initWithAppKey:[self appKey] appSecret:[self appSecret] root:kDBRootAppFolder];
        [DBSession setSharedSession:dbSession];
    }); //I think this might be crashing the app on startup, if so it will allow me to get a bug report
    
    if (errorMsg != nil) {
        [self.alertView showError:[self topMostController] title:NSLocalizedString(@"DROPBOX_CFG_ERROR", nil) subTitle:errorMsg closeButtonTitle:@"OK" duration:0.0];
	}
    
    [self checkForUpdates];
    /*
    NSSetUncaughtExceptionHandler(&HandleException);
    signal(SIGABRT, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGSEGV, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGBUS, SignalHandler);
    signal(SIGPIPE, SignalHandler);
    [self performSelector:@selector(badAccess) withObject:nil afterDelay:3.0];*/
    
    return YES;
}

- (void)badAccess
{
    void (*nullFunction)() = NULL;
    
    nullFunction();
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    NSLog(@"Opening: %@", url);
    if ([[[NSString stringWithFormat:@"%@", url] substringToIndex:2] isEqualToString: @"db"]) {
        NSLog(@"DB");
        if ([[DBSession sharedSession] handleOpenURL:url]) {
            if ([[DBSession sharedSession] isLinked]) {
                [self.alertView showInfo:NSLocalizedString(@"SUCCESS", nil) subTitle:NSLocalizedString(@"SUCCESS_DETAIL", nil) closeButtonTitle:@"Okay!" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"enableDropbox"];
                [CHBgDropboxSync clearLastSyncData];
                [CHBgDropboxSync start];
            }
            return YES;
        }
    } else if (url.isFileURL && [[NSFileManager defaultManager] fileExistsAtPath:url.path] && ([url.path.stringByDeletingLastPathComponent.lastPathComponent isEqualToString:@"Inbox"] || [url.path.stringByDeletingLastPathComponent.lastPathComponent isEqualToString:@"tmp"])) {
        NSLog(@"Zip File");
        NSFileManager *fm = [NSFileManager defaultManager];
        NSError *err = nil;
        if ([url.pathExtension.lowercaseString isEqualToString:@"zip"] || [url.pathExtension.lowercaseString isEqualToString:@"7z"] || [url.pathExtension.lowercaseString isEqualToString:@"rar"]) {
            
            // expand zip
            // create directory to expand
            NSString *dstDir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"extract"];
            if ([fm createDirectoryAtPath:dstDir withIntermediateDirectories:YES attributes:nil error:&err] == NO) {
                NSLog(@"Could not create directory to expand zip: %@ %@", dstDir, err);
                [fm removeItemAtURL:url error:NULL];
                [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"Unable to extract .zip file." closeButtonTitle:@"OK" duration:0.0];
                return NO;
            }
            
            // expand
            NSLog(@"Expanding: %@", url.path);
            NSLog(@"To: %@", dstDir);
            if ([url.pathExtension.lowercaseString isEqualToString:@"zip"]) {
                [SSZipArchive unzipFileAtPath:url.path toDestination:dstDir];
            } else if ([url.pathExtension.lowercaseString isEqualToString:@"7z"]) {
                if (![LZMAExtractor extract7zArchive:url.path tmpDirName:[@"extract" stringByAppendingPathComponent:url.path.lastPathComponent]]) {
                    NSLog(@"Unable to extract 7z");
                    [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"Unable to extract .7z file." closeButtonTitle:@"OK" duration:0.0];
                    return NO;
                }
            } else { //Rar
                NSError *archiveError = nil;
                URKArchive *archive = [[URKArchive alloc] initWithPath:url.path error:&archiveError];
                if (!archive) {
                    NSLog(@"Unable to open rar: %@", archiveError);
                    [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"Unable to read extract .rar file." closeButtonTitle:@"OK" duration:0.0];
                    return NO;
                }
                //Extract
                NSError *error;
                [archive extractFilesTo:dstDir overwrite:YES progress:nil error:&error];
                if (error) {
                    NSLog(@"Unable to extract rar: %@", archiveError);
                    [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"Unable to extract .rar file." closeButtonTitle:@"OK" duration:0.0];
                    return NO;
                }
                
            }
            NSLog(@"Searching");
            NSMutableArray * foundItems = [NSMutableArray array];
            // move .iNDS to documents and .dsv to battery
            for (NSString *path in [fm subpathsAtPath:dstDir]) {
                if ([path.pathExtension.lowercaseString isEqualToString:@"nds"] && ![[path.lastPathComponent substringToIndex:1] isEqualToString:@"."]) {
                    NSLog(@"found ROM in zip: %@", path);
                    [fm moveItemAtPath:[dstDir stringByAppendingPathComponent:path] toPath:[self.documentsPath stringByAppendingPathComponent:path.lastPathComponent] error:NULL];
                   [foundItems addObject:path.lastPathComponent];
                } else if ([path.pathExtension.lowercaseString isEqualToString:@"dsv"]) {
                    NSLog(@"found save in zip: %@", path);
                    [fm moveItemAtPath:[dstDir stringByAppendingPathComponent:path] toPath:[self.batteryDir stringByAppendingPathComponent:path.lastPathComponent] error:NULL];
                    [foundItems addObject:path.lastPathComponent];
                } else {
                    NSLog(@"Discarding: %@", path);
                }
            }
            if (foundItems.count == 0) {
                [self.alertView showError:[self topMostController] title:@"Error!" subTitle:@"No roms or saves found in archive." closeButtonTitle:@"OK" duration:0.0];
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
        }
        // remove inbox (shouldn't be needed)
        [fm removeItemAtPath:[self.documentsPath stringByAppendingPathComponent:@"Inbox"] error:NULL];
        // Clear temp
        NSArray* tmpDirectory = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:NSTemporaryDirectory() error:NULL];
        for (NSString *file in tmpDirectory) {
            [[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithFormat:@"%@%@", NSTemporaryDirectory(), file] error:NULL];
        }
        
        return YES;
    } else {
        [self.alertView showError:[self topMostController] title:@"Error!" subTitle:[NSString stringWithFormat:@"Unable to open file: Unknown Error (%i, %i, %@)", url.isFileURL, [[NSFileManager defaultManager] fileExistsAtPath:url.path], url] closeButtonTitle:@"OK" duration:0.0];
        
    }
    return NO;
}

- (UIViewController*) topMostController
{
    UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (topController.presentedViewController) {
        topController = topController.presentedViewController;
    }
    return topController;
}

- (void)checkForUpdates
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *latestVersion = [NSString stringWithContentsOfURL:[NSURL URLWithString:@"http://www.williamlcobb.com/iNDS/latest.txt"] encoding:NSUTF8StringEncoding error:nil];
        latestVersion = [latestVersion stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        NSString *myVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
        
        if ([latestVersion compare:myVersion options:NSNumericSearch] == NSOrderedDescending && ![[NSUserDefaults standardUserDefaults] objectForKey:myVersion]) {
            NSLog(@"Upgrade Needed lastest<%@> mine<%@>", latestVersion, myVersion);
            dispatch_async(dispatch_get_main_queue(), ^{
                SCLAlertView * alert = [[SCLAlertView alloc] init];
                [alert showInfo:[self topMostController] title:@"Update" subTitle:[NSString stringWithFormat:@"A newer version of iNDS is now avaliable: %@", latestVersion] closeButtonTitle:@"Thanks!" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:myVersion];
            });
        } else {
            //Show Twitter alert
            if (![[NSUserDefaults standardUserDefaults] objectForKey:@"TwitterAlert"]) {
                [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"TwitterAlert"];
            } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] < 5) {
                [[NSUserDefaults standardUserDefaults] setInteger: [[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] + 1 forKey:@"TwitterAlert"];
            } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] == 5) {
                [[NSUserDefaults standardUserDefaults] setInteger:10 forKey:@"TwitterAlert"];
                dispatch_async(dispatch_get_main_queue(), ^{
                    SCLAlertView * alert = [[SCLAlertView alloc] init];
                    alert.iconTintColor = [UIColor whiteColor];
                    alert.shouldDismissOnTapOutside = YES;
                    [alert addButton:@"Follow" actionBlock:^(void) {
                        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/miniroo321"]];
                    }];
                    UIImage * twitterImage = [[UIImage imageNamed:@"Twitter.png"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
                    [alert showCustom:[self topMostController] image:twitterImage color:[UIColor colorWithRed:85/255.0 green:175/255.0 blue:238/255.0 alpha:1] title:@"Love iNDS?" subTitle:@"Show some love and get updates about the newest emulators by following the developer on Twitter!" closeButtonTitle:@"No, Thanks" duration:0.0];
                });
            }
        }
    });
}

- (NSString *)batteryDir
{
    return [self.documentsPath stringByAppendingPathComponent:@"Battery"];
}

- (NSString *)documentsPath
{
    return [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] stringByAppendingPathComponent:@"iNDS"];
}

- (NSString *)oldDocumentsPath //remove later
{
    return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
}

- (NSString *)oldBatteryDir
{
    return [self.oldDocumentsPath stringByAppendingPathComponent:@"Battery"];
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
{
    [[Crashlytics sharedInstance] setObjectValue:game.title forKey:@"GameTitle"];
    [[Crashlytics sharedInstance] setIntValue:(int)savedState forKey:@"SavedState"];
    if (!self.currentEmulatorViewController) {
        iNDSEmulatorViewController *emulatorViewController = (iNDSEmulatorViewController *)[[UIStoryboard storyboardWithName:@"MainStoryboard" bundle:nil] instantiateViewControllerWithIdentifier:@"emulatorView"];
        emulatorViewController.game = game;
        emulatorViewController.saveState = [game pathForSaveStateAtIndex:savedState];
        [AppDelegate sharedInstance].currentEmulatorViewController = emulatorViewController;
        iNDSInitialViewController *rootViewController = (iNDSInitialViewController*)[self topMostController];
        //[rootViewController doSlideIn:nil];
        [rootViewController presentViewController:emulatorViewController animated:YES completion:nil];
    } else {
        self.currentEmulatorViewController.game = game;
        self.currentEmulatorViewController.saveState = [game pathForSaveStateAtIndex:savedState];
        [self.currentEmulatorViewController changeGame];
    }
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

void HandleExceptions(NSException *exception) {
    NSLog(@"The app has encountered an unhandled exception: %@", [exception debugDescription]);
    
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [CHBgDropboxSync start];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}


//THis could be used later to save a game state on crash
- (void)handleException:(NSException *)exception
{
    
    UIAlertView *alert =
    [[UIAlertView alloc] initWithTitle:@"A" message:@"B" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:nil];
    
    NSSetUncaughtExceptionHandler(NULL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    
    [exception raise];
    
}

void HandleException(NSException *exception)
{
    //[exception raise];
    
    NSSetUncaughtExceptionHandler(NULL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    NSLog(@"Uncaught Exception");
}

void SignalHandler(int sig)
{
    NSSetUncaughtExceptionHandler(NULL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    /*UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
    while (topController.presentedViewController) {
        topController = topController.presentedViewController;
    }
    SCLAlertView * alert = [[SCLAlertView alloc] init];
    [alert showError:topController title:@"Crash!" subTitle:@"S.O.S" closeButtonTitle:@"Bye" duration:0.0];*/
}

@end

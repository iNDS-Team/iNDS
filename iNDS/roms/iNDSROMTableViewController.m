//
//  iNDSMasterViewController.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import "iNDSROMTableViewController.h"
#import <DropboxSDK/DropboxSDK.h>
#import "CHBgDropboxSync.h"
#import "iNDSGameTableView.h"
#import "iNDSRomDownloadManager.h"
#import <Crashlytics/Crashlytics.h>
#import "SCLAlertView.h"
@interface iNDSROMTableViewController () {
    NSMutableArray * activeDownloads;
}

@end

@implementation iNDSROMTableViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = @"Roms";
    
    BOOL isDir;
    NSFileManager* fm = [NSFileManager defaultManager];
    
    if (![fm fileExistsAtPath:AppDelegate.sharedInstance.batteryDir isDirectory:&isDir])
    {
        [fm createDirectoryAtPath:AppDelegate.sharedInstance.batteryDir withIntermediateDirectories:NO attributes:nil error:nil];
        NSLog(@"Created Battery");
    } else {
        // move saved states from documents into battery directory
        for (NSString *file in [fm contentsOfDirectoryAtPath:AppDelegate.sharedInstance.documentsPath error:NULL]) {
            if ([file.pathExtension isEqualToString:@"dsv"]) {
                NSError *err = nil;
                [fm moveItemAtPath:[AppDelegate.sharedInstance.documentsPath stringByAppendingPathComponent:file]
                            toPath:[AppDelegate.sharedInstance.batteryDir stringByAppendingPathComponent:file]
                             error:&err];
                if (err) NSLog(@"Could not move %@ to battery dir: %@", file, err);
            }
        }
    }
    
    // Localize the title
    romListTitle.title = NSLocalizedString(@"ROM_LIST", nil);
    
    // watch for changes in documents folder
    docWatchHelper = [DocWatchHelper watcherForPath:AppDelegate.sharedInstance.documentsPath];
    
    // register for notifications
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(reloadGames:) name:iNDSGameSaveStatesChangedNotification object:nil];
    [nc addObserver:self selector:@selector(reloadGames:) name:kDocumentChanged object:docWatchHelper];
    
    activeDownloads = [[iNDSRomDownloadManager sharedManager] activeDownloads];
    
    [self reloadGames:nil];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    [self.tableView reloadData];
}

- (void)viewDidAppear:(BOOL)animated {
    [CHBgDropboxSync start];
    [super viewWillAppear:animated];
    [self checkForUpdates];
}

- (void)reloadGames:(NSNotification*)aNotification
{
    NSUInteger row = [aNotification.object isKindOfClass:[iNDSGame class]] ? [games indexOfObject:aNotification.object] : NSNotFound;
    if (aNotification.object == docWatchHelper) {
        // do it later, the file may not be written yet
        [self performSelector:_cmd withObject:nil afterDelay:2.5];
    }
    if (aNotification == nil || row == NSNotFound) {
        // reload all games
        games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
        [self.tableView reloadData];
    } else {
        // reload single row
        [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:row inSection:0]] withRowAnimation:UITableViewRowAnimationAutomatic];
    }
}

#pragma mark - Table View

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        if (indexPath.section == 0) {  // Del game
            iNDSGame *game = games[indexPath.row];
            if ([[NSFileManager defaultManager] removeItemAtPath:game.path error:NULL]) {
                CLS_LOG(@"Removeing Game");
                games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
                [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            }
        } else {  // Del DL
            CLS_LOG(@"Removeing Download");
            iNDSRomDownload * download = activeDownloads[indexPath.row];
            [[iNDSRomDownloadManager sharedManager] removeDownload:download];
            [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        }
    }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    if (section == 0)
        return games.count;
    return activeDownloads.count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Cell"];
    if (indexPath.section == 0) { // Game
        iNDSGame *game = games[indexPath.row];
        if (game.gameTitle) {
            // use title from ROM
            NSArray *titleLines = [game.gameTitle componentsSeparatedByString:@"\n"];
            cell.textLabel.text = titleLines[0];
            cell.detailTextLabel.text = titleLines.count >= 1 ? titleLines[1] : nil;
        } else {
            // use filename
            cell.textLabel.text = game.title;
            cell.detailTextLabel.text = nil;
        }
        
        cell.imageView.image = game.icon;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    } else { //Download
        iNDSRomDownload * download = activeDownloads[indexPath.row];
        cell.textLabel.text = download.name;
        download.progressLabel = cell.detailTextLabel;
        cell.detailTextLabel.text = @"Waiting...";
        CALayer * progressLayer = [[CALayer alloc] init];
        progressLayer.frame = CGRectMake(0, 0, 0, 5);
        cell.imageView.image = nil;
    }
    return cell;
}

#pragma mark - Select ROMs

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 0) {
        iNDSGame *game = games[indexPath.row];
        iNDSGameTableView * gameInfo = [[iNDSGameTableView alloc] init];
        gameInfo.game = game;
        [self.navigationController pushViewController:gameInfo animated:YES];
        [tableView deselectRowAtIndexPath:indexPath animated:YES];
    } else {
        [tableView deselectRowAtIndexPath:indexPath animated:YES];
    }
}

- (void)checkForUpdates
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *latestVersion = [NSString stringWithContentsOfURL:[NSURL URLWithString:@"http://www.williamlcobb.com/iNDS/latest.txt"] encoding:NSUTF8StringEncoding error:nil];
        latestVersion = [latestVersion stringByReplacingOccurrencesOfString:@"\n" withString:@""];
        NSString *myVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
        
        if ([latestVersion compare:myVersion options:NSNumericSearch] == NSOrderedDescending) {
            NSLog(@"Upgrade Needed lastest<%@> mine<%@>", latestVersion, myVersion);
            dispatch_async(dispatch_get_main_queue(), ^{
                if (![[NSUserDefaults standardUserDefaults] objectForKey:myVersion]) {
                    SCLAlertView * alert = [[SCLAlertView alloc] init];
                    [alert showInfo:self title:@"Update" subTitle:[NSString stringWithFormat:@"A newer version of iNDS is now avaliable: %@", latestVersion] closeButtonTitle:@"Thanks!" duration:0.0];
                    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:myVersion];
                }
            });
        }
        NSLog(@"HIIHI %d", [[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"]);
        //Show Twitter alert
        if (![[NSUserDefaults standardUserDefaults] objectForKey:@"TwitterAlert"]) {
            [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"TwitterAlert"];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] < 5) {
            [[NSUserDefaults standardUserDefaults] setInteger: [[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] + 1 forKey:@"TwitterAlert"];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] == 5) {
            NSLog(@"HEYEYEY");
            [[NSUserDefaults standardUserDefaults] setInteger: [[NSUserDefaults standardUserDefaults] integerForKey:@"TwitterAlert"] + 1 forKey:@"TwitterAlert"];
            dispatch_async(dispatch_get_main_queue(), ^{
                SCLAlertView * alert = [[SCLAlertView alloc] init];
                alert.iconTintColor = [UIColor whiteColor];
                alert.shouldDismissOnTapOutside = YES;
                [alert addButton:@"Follow" actionBlock:^(void) {
                    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/miniroo321"]];
                }];
                UIImage * twitterImage = [[UIImage imageNamed:@"Twitter.png"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
                [alert showCustom:self image:twitterImage color:[UIColor colorWithRed:85/255.0 green:175/255.0 blue:238/255.0 alpha:1] title:@"Love iNDS?" subTitle:@"Show some love and get updates about the newest emulators by following the developer on Twitter!" closeButtonTitle:@"No, Thanks" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"TwitterAlert"];
            });
        }
    });
}


@end
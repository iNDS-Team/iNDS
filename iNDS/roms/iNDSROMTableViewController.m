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
#import "SCLAlertView.h"
//#import <Crashlytics/Crashlytics.h>
#import "MHWDirectoryWatcher.h"

@interface iNDSROMTableViewController () {
    NSMutableArray * activeDownloads;
    MHWDirectoryWatcher * docWatchHelper;
}

@end

@implementation iNDSROMTableViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:NO];
    self.navigationItem.title = NSLocalizedString(@"ROM_LIST", nil);
    //self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
    
    // watch for changes in documents folder
    docWatchHelper = [MHWDirectoryWatcher directoryWatcherAtPath:AppDelegate.sharedInstance.documentsPath
                                                        callback:^{
                                                            [self reloadGames:self];
                                                        }];
    
    activeDownloads = [[iNDSRomDownloadManager sharedManager] activeDownloads];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:NO];
    [self reloadGames:nil];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [AppDelegate.sharedInstance startBackgroundProcesses];
}


- (void)reloadGames:(id)sender
{
    NSLog(@"Reloading");
    if (sender == self) {
        // do it later, the file may not be written yet
        [self performSelector:_cmd withObject:nil afterDelay:1.5];
    } else  {
        // reload all games
        games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.tableView reloadData];
        });
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
            //CLS_LOG(@"Removeing Game");
            iNDSGame *game = games[indexPath.row];
            if ([[NSFileManager defaultManager] removeItemAtPath:game.path error:NULL]) {
                games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
                [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            }
        } else {
            //CLS_LOG(@"Removeing Download");
            if (indexPath.row >= activeDownloads.count) return;
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
    UITableViewCell *cell;
    if (indexPath.section == 0) { // Game
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"iNDSGame"];
        if (indexPath.row >= games.count) return [UITableViewCell new];
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
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"iNDSDownload"];
        if (indexPath.row >= activeDownloads.count) return [UITableViewCell new];
        iNDSRomDownload * download = activeDownloads[indexPath.row];
        cell.textLabel.text = download.name;
        download.progressLabel = cell.detailTextLabel;
        cell.detailTextLabel.text = @"Waiting...";
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



@end
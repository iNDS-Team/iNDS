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
#import <Crashlytics/Crashlytics.h>

@interface iNDSROMTableViewController () {
    NSMutableArray * activeDownloads;
}

@end

@implementation iNDSROMTableViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.title = @"Roms";
    self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
    
    BOOL isDir;
    NSFileManager* fm = [NSFileManager defaultManager];
    
    if (![fm fileExistsAtPath:AppDelegate.sharedInstance.batteryDir isDirectory:&isDir])
    {
        [fm createDirectoryAtPath:AppDelegate.sharedInstance.batteryDir withIntermediateDirectories:NO attributes:nil error:nil];
        NSLog(@"Created Battery");
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
    
    
}

- (UIStatusBarStyle) preferredStatusBarStyle
{
    return UIStatusBarStyleLightContent;
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
        [self.tableView reloadData];
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
            CLS_LOG(@"Removeing Game");
            iNDSGame *game = games[indexPath.row];
            if ([[NSFileManager defaultManager] removeItemAtPath:game.path error:NULL]) {
                games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
                [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            }
        } else {
            CLS_LOG(@"Removeing Download");
            if (indexPath.row > activeDownloads.count - 1) return;
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
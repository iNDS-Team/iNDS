//
//  iNDSMasterViewController.m
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import "iNDSROMTableViewController.h"
//#import <DropboxSDK/DropboxSDK.h>
#import "CHBgDropboxSync.h"
#import "iNDSGameTableView.h"
#import "iNDSRomDownloadManager.h"
#import "SCLAlertView.h"
#import "MHWDirectoryWatcher.h"
#import "WCEasySettingsViewController.h"
#import "WCBuildStoreClient.h"
#import <AVFoundation/AVFoundation.h>
#import <SDWebImage/UIView+WebCache.h>
#import <SDWebImage/UIImageView+WebCache.h>
#import <SDWebImage/SDWebImagePrefetcher.h>

@interface iNDSROMTableViewController () {
    NSMutableArray * activeDownloads;
    MHWDirectoryWatcher * docWatchHelper;
}

@end

@implementation iNDSROMTableViewController

- (void)configUI
{
    self.tableView.tableFooterView = [UIView new];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self configUI];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:NO];
    self.navigationItem.title = NSLocalizedString(@"ROM_LIST", nil);
    activeDownloads = [[iNDSRomDownloadManager sharedManager] activeDownloads];
#ifdef DEBUG
    self.title = @"DEBUG MODE";
    if (![[NSUserDefaults standardUserDefaults] objectForKey:@"debugAlert"]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            SCLAlertView * alert = [[SCLAlertView alloc] init];
            alert.iconTintColor = [UIColor whiteColor];
            alert.shouldDismissOnTapOutside = YES;
            [alert showWarning:self title:@"Debug Mode" subTitle:@"Warning you are running iNDS in debug mode which is very slow. Please change the build configuration to Release if you are not planning on debugging." closeButtonTitle:@"Got it" duration:0.0];
        });
    }
#endif
    
    [SDWebImageManager sharedManager].delegate = self;
    
    [self prefetchImages];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(userRequestedToPlayROM:) name:iNDSUserRequestedToPlayROMNotification object:nil];
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
    [self initMicrophone];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [AppDelegate.sharedInstance startBackgroundProcesses];
    // watch for changes in documents folder
    docWatchHelper = [MHWDirectoryWatcher directoryWatcherAtPath:AppDelegate.sharedInstance.documentsPath
                                                        callback:^{
                                                            [self reloadGames:self];
                                                        }];
}

- (void)initMicrophone {
    switch ([[AVAudioSession sharedInstance] recordPermission]) {
        case AVAudioSessionRecordPermissionGranted:
            printf("Microphone enabled\n");
            break;
        case AVAudioSessionRecordPermissionDenied:
            printf("Microphone AVAudioSessionRecordPermissionDenied\n");
            break;
        case AVAudioSessionRecordPermissionUndetermined:
            // This is the initial state before a user has made any choice
            // You can use this spot to request permission here if you want
            printf("Microphone AVAudioSessionRecordPermissionUndetermined\n");
            [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
                if (granted) {
                    printf("Microphone enabled\n");
                }
                else {
                    printf("Microphone disabled\n");
                }
            }];
            break;
        default:
            printf("Microphone unknown\n");
            break;
    }
}

- (void) prefetchImages {
    // Prefetch images
    /*
    NSMutableArray *urls = [[NSMutableArray alloc] init];
    for (int i = 0; i < games.count; i++) {
        [urls addObject:[games[i] imageURL]];
    }
    NSLog(@"%@", urls);
    [[SDWebImagePrefetcher sharedImagePrefetcher] prefetchURLs:urls];
     */
}


- (IBAction)openSettings:(id)sender
{
    WCEasySettingsViewController *settingsView = AppDelegate.sharedInstance.settingsViewController;
    settingsView.title = @"Emulator Settings";
    
    UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:settingsView];
    UIColor *globalTint = [[[UIApplication sharedApplication] delegate] window].tintColor;
    
    nav.navigationBar.barTintColor = globalTint;
    nav.navigationBar.translucent = NO;
    nav.navigationBar.tintColor = [UIColor whiteColor];
    [nav.navigationBar setTitleTextAttributes:@{NSForegroundColorAttributeName : [UIColor whiteColor]}];
    nav.modalPresentationStyle = UIModalPresentationFullScreen;

    [self presentViewController:nav animated:YES completion:nil];
}

- (void)reloadGames:(id)sender
{
    NSLog(@"Reloading");
    [self prefetchImages];
    if (sender == self) {
        // do it later, the file may not be written yet
        [self performSelector:_cmd withObject:nil afterDelay:1];
    } else  {
        // reload all games
        NSArray *games2 = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
        NSMutableSet *setA = [NSMutableSet setWithArray:games];
        NSSet *setB = [NSSet setWithArray:games2];
        [setA intersectSet:setB];
        [setA unionSet:setB];
        games = [setA allObjects];
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

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath { }

-(NSArray *)tableView:(UITableView *)tableView editActionsForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewRowAction *renameAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"Rename" handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
        
        UITextField *textField = [alert addTextField:@""];
        iNDSGame *game = self->games[indexPath.row];
        textField.placeholder = game.origTitle;
        
        [alert addButton:@"Rename" actionBlock:^(void) {
            [game setAltTitle:textField.text];
            [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        }];
        
        [alert showEdit:self title:@"Rename" subTitle:@"Rename Game" closeButtonTitle:@"Cancel" duration:0.0f];
    }];
    renameAction.backgroundColor = [UIColor colorWithRed:1.00 green:0.76 blue:0.03 alpha:1.0];
    
    UITableViewRowAction *deleteAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"Delete"  handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        if (indexPath.section == 0) {  // Del game
            iNDSGame *game = self->games[indexPath.row];
            if ([[NSFileManager defaultManager] removeItemAtPath:game.path error:NULL]) {
                self->games = [iNDSGame gamesAtPath:AppDelegate.sharedInstance.documentsPath saveStateDirectoryPath:AppDelegate.sharedInstance.batteryDir];
                [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            }
        } else {
            if (indexPath.row >= self->activeDownloads.count) return;
            iNDSRomDownload * download = self->activeDownloads[indexPath.row];
            [[iNDSRomDownloadManager sharedManager] removeDownload:download];
            [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        }
    }];
    
    deleteAction.backgroundColor = [UIColor redColor];
    
    return @[deleteAction, renameAction];
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

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 88.0;
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
            cell.detailTextLabel.text = titleLines.count > 1 ? titleLines[1] : nil;
        } else {
            // use filename
            cell.textLabel.text = game.title;
            cell.detailTextLabel.text = nil;
        }
        NSString *gameIcon = [game imageURL];
        
        if ([gameIcon isEqualToString:@"none"]) {
            cell.imageView.image = [UIImage imageNamed:@"smpte.png"];
        } else {
            [cell.imageView sd_setShowActivityIndicatorView:true];
            [cell.imageView sd_setIndicatorStyle:UIActivityIndicatorViewStyleWhite];
            [cell.imageView sd_setImageWithURL:[NSURL URLWithString:gameIcon] placeholderImage:[UIImage imageNamed:@"smpte.png"]];
        }
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    } else { //Download
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"iNDSDownload"];
        if (indexPath.row >= activeDownloads.count) return [UITableViewCell new];
        iNDSRomDownload * download = activeDownloads[indexPath.row];
        cell.textLabel.text = download.name;
        download.progressLabel = cell.detailTextLabel;
        cell.detailTextLabel.text = @"Waiting...";
        cell.imageView.image = [UIImage imageNamed:@"smpte.png"];
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

#pragma mark - Resize Downloaded Images

- (UIImage *) imageManager:(SDWebImageManager *)imageManager transformDownloadedImage:(UIImage *)image withURL:(NSURL *)imageURL {
    float newHeight = 72;
    float scaleFactor = newHeight / image.size.height;
    float newWidth = image.size.width * scaleFactor;
    
    UIGraphicsBeginImageContext(CGSizeMake(newWidth, newHeight));
    [image drawInRect:CGRectMake(0, 0, newWidth, newHeight)];
    UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return newImage;
}

#pragma mark - Start Game from URL

- (void) userRequestedToPlayROM:(NSNotification *) notification {
    iNDSGame *game = notification.object;
    
    [AppDelegate.sharedInstance startGame:game withSavedState:-1];

}

@end


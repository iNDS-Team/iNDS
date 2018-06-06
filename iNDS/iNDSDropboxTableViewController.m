//
//  iNDSDropboxTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 4/22/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSDropboxTableViewController.h"
#import "WCEasySettingsSwitch.h"

#import "CHBgDropboxSync.h"
//#import <DropboxSDK/DropboxSDK.h>
#import "SCLAlertView.h"

@interface iNDSDropboxTableViewController () {
    WCEasySettingsSwitch    *dropBoxSwitch;
    NSString                *userName;
}

@end

@implementation iNDSDropboxTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    
    
    dropBoxSwitch = [[WCEasySettingsSwitch alloc] initWithIdentifier:@"enableDropbox" title:@"Enable Dropbox Sync"];
    dropBoxSwitch.readOnlyIdentifier = YES;
    __weak id weakSelf = self;
    [dropBoxSwitch setEnableBlock:^{
        //        [[DBSession sharedSession] linkFromController:weakSelf];
        [DBClientsManager authorizeFromController:[UIApplication sharedApplication] controller:weakSelf openURL:^(NSURL *url) {
            [[UIApplication sharedApplication] openURL:url];
        }];
    }];
    [dropBoxSwitch setDisableBlock:^{
        [CHBgDropboxSync forceStopIfRunning];
        [CHBgDropboxSync clearLastSyncData];
        //        [[DBSession sharedSession] unlinkAll];
        [DBClientsManager unlinkAndResetClients];
        SCLAlertView * alert = [[SCLAlertView alloc] init];
        [alert showInfo:weakSelf title:NSLocalizedString(@"UNLINKED", nil) subTitle:NSLocalizedString(@"UNLINKED_DETAIL", nil) closeButtonTitle:@"Okay!" duration:0.0];
        
        [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"enableDropbox"];
        userName = @"";
        dispatch_async(dispatch_get_main_queue(), ^{
            NSRange range = NSMakeRange(0, [self numberOfSectionsInTableView:self.tableView]);
            NSIndexSet *sections = [NSIndexSet indexSetWithIndexesInRange:range];
            [self.tableView reloadSections:sections withRowAnimation:UITableViewRowAnimationAutomatic];
        });
    }];
    
    [self.tableView registerClass:[WCEasySettingsSwitchCell class] forCellReuseIdentifier:dropBoxSwitch.cellIdentifier];
}

- (void)loadedAccountInfo:(DBUSERSFullAccount *)info {
    userName = info.email;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSRange range = NSMakeRange(0, [self numberOfSectionsInTableView:self.tableView]);
        NSIndexSet *sections = [NSIndexSet indexSetWithIndexesInRange:range];
        [self.tableView reloadSections:sections withRowAnimation:UITableViewRowAnimationAutomatic];
    });
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    DBUserClient *client = [DBClientsManager authorizedClient];
    [[client.usersRoutes getCurrentAccount]
     setResponseBlock:^(DBUSERSFullAccount * result, DBNilObject * _Nullable routeError, DBRequestError * _Nullable networkError) {
         if (result) {
             NSLog(@"%@\n", result);
             [self loadedAccountInfo:result];
         } else {
             NSLog(@"%@\n%@\n", routeError, networkError);
         }
     }];
}



- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

#pragma mark - Table view data source

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.row == 0) {
        WCEasySettingsSwitchCell * cell = [tableView dequeueReusableCellWithIdentifier:dropBoxSwitch.cellIdentifier];
        cell.controller = dropBoxSwitch;
        return cell;
    } else {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Account"];
        if (!cell)
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"Account"];
        cell.textLabel.text = @"Account Name";
        cell.detailTextLabel.text = userName;
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.clipsToBounds = YES;
        NSLog(@"Uname %@", userName);
        return cell;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0) {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"enableDropbox"] && userName.length > 0)
            return 2;
        return 1;
    }
    return 0;
}

@end

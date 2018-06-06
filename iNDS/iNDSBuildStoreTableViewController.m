//
//  iNDSBuildStoreTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 5/5/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSBuildStoreTableViewController.h"
#import "WCEasySettingsSwitch.h"
#import "WCBuildStoreClient.h"

@interface iNDSBuildStoreTableViewController () {
    WCEasySettingsSwitch    *buildStoreSwitch;
    WCBuildStoreClient      *client;
}

@end

@implementation iNDSBuildStoreTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    client = [WCBuildStoreClient sharedInstance];
    //client.delegate = self;
    
    buildStoreSwitch = [[WCEasySettingsSwitch alloc] initWithIdentifier:@"enableBuildstore" title:@"Enable Buildstore Updates"];
    buildStoreSwitch.readOnlyIdentifier = YES;
    // retain warnings can be ignored because this controller is never deallocated
    [buildStoreSwitch setEnableBlock:^{
        [client linkFromController:self];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
    }];
    [buildStoreSwitch setDisableBlock:^{
        [client unlink];
    }];
    
    [self.tableView registerClass:[WCEasySettingsSwitchCell class] forCellReuseIdentifier:buildStoreSwitch.cellIdentifier];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    [[NSUserDefaults standardUserDefaults] setBool:client.linked forKey:@"enableBuildstore"];
    [self.tableView reloadData];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

#pragma mark - Table view data source

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.row == 0) {
        WCEasySettingsSwitchCell * cell = [tableView dequeueReusableCellWithIdentifier:buildStoreSwitch.cellIdentifier];
        cell.controller = buildStoreSwitch;
        return cell;
    } else {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Account"];
        if (!cell)
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"Account"];
        cell.textLabel.text = @"Account Name";
        cell.detailTextLabel.text = @"Meep";
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.clipsToBounds = YES;
        return cell;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 1;
//    
//    if (section == 0) {
//        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"enableDropbox"])
//            return 2;
//        return 1;
//    }
//    return 0;
}

@end

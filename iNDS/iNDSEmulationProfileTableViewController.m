//
//  iNDSEmulationProfileTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/23/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSEmulationProfileTableViewController.h"
#import "iNDSEmulationProfile.h"
#import "AppDelegate.h"
@interface iNDSEmulationProfileTableViewController () {
    NSArray * profiles;
}

@end

@implementation iNDSEmulationProfileTableViewController

-(void) viewDidLoad
{
    [super viewDidLoad];
    profiles = [iNDSEmulationProfile profilesAtPath:AppDelegate.sharedInstance.batteryDir];
}

#pragma mark - Table View

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return YES;
}


- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 1) {
        if (editingStyle == UITableViewCellEditingStyleDelete) {
            iNDSEmulationProfile * profile = profiles[indexPath.row];
            if ([profile deleteProfile]) {
                profiles = [iNDSEmulationProfile profilesAtPath:AppDelegate.sharedInstance.batteryDir];
                [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                if (profile == AppDelegate.sharedInstance.currentEmulatorViewController.profile) { //Just deleted current profile
                    //Load default
                    [AppDelegate.sharedInstance.currentEmulatorViewController loadProfile:[[iNDSEmulationProfile alloc] initWithProfileName:@"Default"]];
                }
            } else {
                NSLog(@"Error! unable to delete save state");
            }
        }
    }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    if (section == 0) return 1;
    return profiles.count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSLog(@"Hey");
    UITableViewCell* cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"Cell"];
    if (indexPath.section == 0) {
        cell.textLabel.text = @"Default";
        //cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    else if (indexPath.section == 1) {
        iNDSEmulationProfile * profile = profiles[indexPath.row];
        cell.textLabel.text = profile.name;
        if (profile == AppDelegate.sharedInstance.currentEmulatorViewController.profile) { //Current profile
            cell.accessoryType = UITableViewCellAccessoryCheckmark;
        } else {
            cell.accessoryType = UITableViewCellAccessoryNone;
        }
    }
    return cell;
}

#pragma mark - Select ROMs

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    iNDSEmulationProfile * profile;
    if (indexPath.section == 0) {
        profile = [[iNDSEmulationProfile alloc] initWithProfileName:@"Default"];
    } else if (indexPath.section == 1) {
        profile = profiles[indexPath.row];
    }
    [AppDelegate.sharedInstance.currentEmulatorViewController loadProfile:profile];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [profile ajustLayout];
    [self.navigationController popViewControllerAnimated:YES];
}


@end

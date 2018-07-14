//
//  iNDSProfileEditorTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/26/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSProfileEditorTableViewController.h"
#import "AppDelegate.h"
#import "iNDSEmulationProfile.h"
@interface iNDSProfileEditorTableViewController () {
    BOOL inEditingMode;
    iNDSEmulatorViewController * emulationController;
}

@end

@implementation iNDSProfileEditorTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    emulationController = [AppDelegate sharedInstance].currentEmulatorViewController;
    self.navigationItem.title = emulationController.profile.name;
    
    UIBarButtonItem * xButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemStop target:AppDelegate.sharedInstance.currentEmulatorViewController action:@selector(toggleSettings:)];
    xButton.imageInsets = UIEdgeInsetsMake(7, 3, 7, 0);
    self.navigationItem.rightBarButtonItem = xButton;
    UITapGestureRecognizer* tapRecon = [[UITapGestureRecognizer alloc] initWithTarget:AppDelegate.sharedInstance.currentEmulatorViewController action:@selector(toggleSettings:)];
    tapRecon.numberOfTapsRequired = 2;
    //[self.navigationController.navigationBar addGestureRecognizer:tapRecon];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell * cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cell"];
    if (inEditingMode) {
        switch (indexPath.row) {
            case 0:
                cell.textLabel.text = @"Save Profile";
                break;
            case 1:
                cell.textLabel.text = @"Discard Changes";
                break;
            default:
                break;
        }
    } else {
        switch (indexPath.row) {
            case 0:
                cell.textLabel.text = @"Edit Profile Layout";
                break;
            case 1:
                cell.textLabel.text = @"Rename Profile";
                break;
            case 2:
                cell.textLabel.text = @"New Profile";
                break;
            case 3:
                cell.textLabel.text = @"Duplicate Profile";
                break;
            default:
                break;
        }
    }
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    iNDSEmulationProfile * currentProfile = emulationController.profile;
    if (!inEditingMode) {
        if (indexPath.row == 0) { //Edit profile
            self.navigationItem.hidesBackButton = YES;
            [emulationController enterEditMode];
            inEditingMode = YES;
        } else if (indexPath.row == 1) {//Rename
            [currentProfile deleteProfile];
            [currentProfile saveProfileWithCancel:NO];
        } else if (indexPath.row == 2) {//New
            self.navigationItem.hidesBackButton = YES;
            iNDSEmulationProfile * defaultProfile = [[iNDSEmulationProfile alloc] initWithProfileName:@"iNDSDefaultProfile"];
            [emulationController loadProfile:defaultProfile];
            [emulationController enterEditMode];
            inEditingMode = YES;
        } else if (indexPath.row == 3) {//Duplicate
            [currentProfile saveProfileWithCancel:YES];
        }
    } else {
        if (indexPath.row == 0 ) {//Save
            self.navigationItem.hidesBackButton = NO;
            [emulationController.profile saveProfileWithCancel:NO];
            inEditingMode = NO;
        } else { //Discard
            //Just reload from file
            iNDSEmulationProfile *reloadedProfile;
            if ([currentProfile.name isEqualToString:@"iNDSDefaultProfile"]) {
                reloadedProfile = [[iNDSEmulationProfile alloc] initWithProfileName:@"iNDSDefaultProfile"];
            } else {
                NSString * profilePath = [iNDSEmulationProfile pathForProfileName:currentProfile.name];
                reloadedProfile = [iNDSEmulationProfile profileWithPath:profilePath];
            }
            
            inEditingMode = NO;
            [emulationController exitEditMode];
            [emulationController loadProfile:reloadedProfile];
            [emulationController.profile ajustLayout];
            
            
            [self.navigationController popViewControllerAnimated:YES];
        }
        
    }
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [tableView reloadData];
}

#pragma mark - Table view data source
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (inEditingMode) return 2;
    return 4;
}


@end

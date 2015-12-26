//
//  iNDSSettingsTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/21/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSSettingsTableViewController.h"
#import "AppDelegate.h"
#import "iNDSEmulatorViewController.h"
#import "iNDSEmulatorSettingsViewController.h"
#import "iNDSEmulationProfile.h"
@interface iNDSSettingsTableViewController () {
    iNDSEmulatorViewController * emulationController;
    
    BOOL inEditingMode;
}
@end

@implementation iNDSSettingsTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated
{
    emulationController = [AppDelegate sharedInstance].currentEmulatorViewController;
    self.romName.text = [AppDelegate sharedInstance].currentEmulatorViewController.game.gameTitle;
    self.layoutName.text = [[NSUserDefaults standardUserDefaults] stringForKey:@"currentProfile"];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)speedChanged:(UISegmentedControl*) control
{
    int translation[] = {1, 2, 4};
    emulationController.speed = translation[control.selectedSegmentIndex];
    [emulationController toggleSettings:self];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    switch (indexPath.row) {
        case 2: //Edit Layout
            if (!inEditingMode) {
                [emulationController enterEditMode];
                inEditingMode = YES;
                [self.layoutLabel performSelector:@selector(setText:) withObject:@"Save Layout" afterDelay:0.3];
            } else {
                [emulationController.profile saveProfile];
                [emulationController exitEditMode];
                inEditingMode = NO;
                [self.layoutLabel performSelector:@selector(setText:) withObject:@"Edit Layout" afterDelay:0.3];
            }
            break;
        
        case 3: //Save State
            [emulationController newSaveState]; //Request that the controller save
        default:
            break;
    }
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([segue.identifier isEqualToString:@"EmulatorSettings"]) {
        iNDSEmulatorSettingsViewController * vc = segue.destinationViewController;
        vc.navigationItem.leftBarButtonItems = nil;
    }
}

@end

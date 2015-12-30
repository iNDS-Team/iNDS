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
}
@end

@implementation iNDSSettingsTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.navigationController.delegate = self;
}

- (void)viewWillAppear:(BOOL)animated
{
    emulationController = [AppDelegate sharedInstance].currentEmulatorViewController;
    self.romName.text = [AppDelegate sharedInstance].currentEmulatorViewController.game.gameTitle;
    self.layoutName.text = [[NSUserDefaults standardUserDefaults] stringForKey:@"currentProfile"];
}


- (void)navigationController:(UINavigationController *)navigationController didShowViewController:(UIViewController *)viewController animated:(BOOL)animated
{
    UITableViewController * tableController = (UITableViewController *)viewController;
    
    //Uncomment for animation
    //[emulationController setSettingsHeight:tableController.tableView.contentSize.height + 44];
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
        case 3: //Save State
            [emulationController newSaveState]; //Request that the controller save
            break;
        case 8: //Reload
            [emulationController reloadEmulator]; //Request that the controller save
            break;
        default:
            break;
    }
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (IBAction)close:(id)sender
{
    [emulationController toggleSettings:self];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([segue.identifier isEqualToString:@"EmulatorSettings"]) {
        iNDSEmulatorSettingsViewController * vc = segue.destinationViewController;
        vc.navigationItem.leftBarButtonItems = nil;
    }
}

@end

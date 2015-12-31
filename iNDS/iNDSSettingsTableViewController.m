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
#import <QuartzCore/QuartzCore.h>
@interface iNDSSettingsTableViewController () {
    iNDSEmulatorViewController * emulationController;
    
    UIImageView     *syncImageView;
    UIBarButtonItem *barItem;
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
    
    //Sync Image
    UIImage *syncImage = [[UIImage imageNamed:@"Sync.png"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    syncImageView = [[UIImageView alloc] initWithImage:syncImage];
    syncImageView.tintColor = [UIColor whiteColor];
    syncImageView.autoresizingMask = UIViewAutoresizingNone;
    syncImageView.contentMode = UIViewContentModeCenter;
    [syncImageView setHidden:YES];
    
    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    button.frame = CGRectMake(0, 0, 40, 40);
    [button addSubview:syncImageView];
    [button addTarget:self action:@selector(animate) forControlEvents:UIControlEventTouchUpInside];
    syncImageView.center = button.center;
    barItem = [[UIBarButtonItem alloc] initWithCustomView:button];
    self.navigationItem.rightBarButtonItem = barItem;
    
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(showSync) name:@"iNDSDropboxSyncStarted" object:nil];
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(hideSync) name:@"iNDSDropboxSyncEnded" object:nil];
    
}


- (void)showSync
{
    [syncImageView setHidden:NO];
    CABasicAnimation* rotationAnimation;
    rotationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    rotationAnimation.toValue = [NSNumber numberWithFloat: M_PI * 2.0 * -100];
    rotationAnimation.duration = 100;
    rotationAnimation.cumulative = YES;
    rotationAnimation.repeatCount = HUGE_VALF;
    [syncImageView.layer addAnimation:rotationAnimation forKey:@"rotationAnimation"];
}

- (void)hideSync
{
    [syncImageView.layer removeAllAnimations];
    [syncImageView setHidden:YES];
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

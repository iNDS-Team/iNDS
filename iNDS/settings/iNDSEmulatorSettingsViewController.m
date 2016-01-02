//
//  Settings.m
//  iNDS
//
//  Created by Riley Testut on 7/5/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "AppDelegate.h"
#import "iNDSEmulatorSettingsViewController.h"
#import <DropboxSDK/DropboxSDK.h>
#import "SCLAlertView.h"
#import "CHBgDropboxSync.h"

@interface iNDSEmulatorSettingsViewController ()

@property (weak, nonatomic) IBOutlet UINavigationItem *settingsTitle;

@property (weak, nonatomic) IBOutlet UILabel *frameSkipLabel;
@property (weak, nonatomic) IBOutlet UILabel *disableSoundLabel;

@property (weak, nonatomic) IBOutlet UISegmentedControl *frameSkipControl;
@property (weak, nonatomic) IBOutlet UISwitch *disableSoundSwitch;

@property (weak, nonatomic) IBOutlet UILabel *controlPadStyleLabel;
@property (weak, nonatomic) IBOutlet UILabel *controlOpacityLabel;

@property (weak, nonatomic) IBOutlet UISegmentedControl *controlPadStyleControl;
@property (weak, nonatomic) IBOutlet UISlider *controlOpacitySlider;

@property (weak, nonatomic) IBOutlet UILabel *showFPSLabel;
@property (weak, nonatomic) IBOutlet UILabel *autoSaveLabel;

@property (weak, nonatomic) IBOutlet UISwitch *showFPSSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *autoSaveSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *fullScreenSwitch;

@property (weak, nonatomic) IBOutlet UILabel *synchSoundLabel;
@property (weak, nonatomic) IBOutlet UISwitch *synchSoundSwitch;

@property (weak, nonatomic) IBOutlet UISwitch *enableJITSwitch;

@property (weak, nonatomic) IBOutlet UILabel *vibrateLabel;
@property (weak, nonatomic) IBOutlet UISwitch *vibrateSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *bumperSwitch;

@property (weak, nonatomic) IBOutlet UILabel *dropboxLabel;

@property (weak, nonatomic) IBOutlet UISwitch *dropboxSwitch;
@property (weak, nonatomic) IBOutlet UISwitch *cellularSwitch;
@property (weak, nonatomic) IBOutlet UILabel *accountLabel;

- (IBAction)controlChanged:(id)sender;

@end

@implementation iNDSEmulatorSettingsViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.settingsTitle.title = NSLocalizedString(@"SETTINGS", nil);
    
    self.frameSkipLabel.text = NSLocalizedString(@"FRAME_SKIP", nil);
    self.disableSoundLabel.text = NSLocalizedString(@"DISABLE_SOUND", nil);
    //self.showPixelGridLabel.text = NSLocalizedString(@"OVERLAY_PIXEL_GRID", nil);

    self.controlPadStyleLabel.text = NSLocalizedString(@"CONTROL_PAD_STYLE", nil);
    self.controlOpacityLabel.text = NSLocalizedString(@"CONTROL_OPACITY_PORTRAIT", nil);
    
    self.dropboxLabel.text = NSLocalizedString(@"ENABLE_DROPBOX", nil);
    self.accountLabel.text = NSLocalizedString(@"NOT_LINKED", nil);
    
    self.showFPSLabel.text = NSLocalizedString(@"SHOW_FPS", nil);
    self.vibrateLabel.text = NSLocalizedString(@"VIBRATION", nil);

    [self.frameSkipControl setTitle:NSLocalizedString(@"AUTO", nil) forSegmentAtIndex:5];

    [self.controlPadStyleControl setTitle:NSLocalizedString(@"DPAD", nil) forSegmentAtIndex:0];
    [self.controlPadStyleControl setTitle:NSLocalizedString(@"JOYSTICK", nil) forSegmentAtIndex:1];
    
    
    UIView *hiddenSettingsTapView = [[UIView alloc] initWithFrame:CGRectMake(245, 0, 75, 44)];
    
    UIBarButtonItem *hiddenSettingsBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:hiddenSettingsTapView];
    self.navigationItem.rightBarButtonItem = hiddenSettingsBarButtonItem;
    
    UITapGestureRecognizer *tapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(revealHiddenSettings:)];
    tapGestureRecognizer.numberOfTapsRequired = 3;
    [hiddenSettingsTapView addGestureRecognizer:tapGestureRecognizer];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSInteger frameSkip = [defaults integerForKey:@"frameSkip"];
    self.frameSkipControl.selectedSegmentIndex = frameSkip < 0 ? 5 : frameSkip;
    self.disableSoundSwitch.on = [defaults boolForKey:@"disableSound"];
    
    self.controlPadStyleControl.selectedSegmentIndex = [defaults integerForKey:@"controlPadStyle"];
    self.controlOpacitySlider.value = [defaults floatForKey:@"controlOpacity"];
    
    self.showFPSSwitch.on = [defaults boolForKey:@"showFPS"];
    self.autoSaveSwitch.on = [defaults boolForKey:@"periodicSave"];
    self.fullScreenSwitch.on = [defaults boolForKey:@"fullScreenSettings"];
    self.synchSoundSwitch.on = [defaults boolForKey:@"synchSound"];
    
    self.enableJITSwitch.on = [defaults boolForKey:@"enableLightningJIT"];
    self.vibrateSwitch.on = [defaults boolForKey:@"vibrate"];
    self.bumperSwitch.on = [defaults boolForKey:@"volumeBumper"];
    
    self.dropboxSwitch.on = [defaults boolForKey:@"enableDropbox"];
    self.cellularSwitch.on = [defaults boolForKey:@"enableDropboxCellular"];
    
    if ([defaults boolForKey:@"enableDropbox"] == true) {
        self.accountLabel.text = NSLocalizedString(@"LINKED", nil);
    }
    [self appDidBecomeActive];
}



- (NSString *)tableView:(UITableView *)tableView  titleForHeaderInSection:(NSInteger)section
{
    NSString *sectionName;
    switch (section)
    {
        case 0:
            sectionName = NSLocalizedString(@"EMULATOR", nil);
            break;
        case 1:
            sectionName = NSLocalizedString(@"CONTROLS", nil);
            break;
        case 2:
            sectionName = @"Dropbox";
            break;
        case 3:
            sectionName = NSLocalizedString(@"DEVELOPER", nil);
            break;
        case 4:
            sectionName = @"Info";
            break;
        case 5:
            sectionName = NSLocalizedString(@"EXPERIMENTAL", nil);
            break;
        default:
            sectionName = @"";
            break;
    }
    return sectionName;
}

- (NSString *)tableView:(UITableView *)tableView  titleForFooterInSection:(NSInteger)section
{
    NSString *sectionName;
    NSString *myVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    switch (section)
    {
        case 0:
            //sectionName = NSLocalizedString(@"OVERLAY_PIXEL_GRID_DETAIL", nil);
            sectionName = @"Periodic Save will automatically save the game state every few minutes and will store a limited number of saves.";
            break;
        case 2:
            sectionName = NSLocalizedString(@"ENABLE_DROPBOX_DETAIL", nil);
            break;
        case 4:
            sectionName = [NSString stringWithFormat:@"Version %@", myVersion];
            break;
        case 5:
            sectionName = NSLocalizedString(@"ARMLJIT_DETAIL", nil);
            break;
        default:
            sectionName = @"";
            break;
    }
    return sectionName;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)controlChanged:(id)sender
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (sender == self.frameSkipControl) {
        NSInteger frameSkip = self.frameSkipControl.selectedSegmentIndex;
        if (frameSkip == 5) frameSkip = -1;
        [defaults setInteger:frameSkip forKey:@"frameSkip"];
    } else if (sender == self.disableSoundSwitch) {
        [defaults setBool:self.disableSoundSwitch.on forKey:@"disableSound"];
    } else if (sender == self.synchSoundSwitch) {
        [defaults setBool:self.synchSoundSwitch.on forKey:@"synchSound"];
    } else if (sender == self.autoSaveSwitch) {
        [defaults setBool:self.autoSaveSwitch.on forKey:@"periodicSave"];
    } else if (sender == self.fullScreenSwitch) {
        [defaults setBool:self.fullScreenSwitch.on forKey:@"fullScreenSettings"];
    } else if (sender == self.controlPadStyleControl) {
        [defaults setInteger:self.controlPadStyleControl.selectedSegmentIndex forKey:@"controlPadStyle"];
    } else if (sender == self.controlOpacitySlider) {
        [defaults setFloat:self.controlOpacitySlider.value forKey:@"controlOpacity"];
    } else if (sender == self.showFPSSwitch) {
        [defaults setBool:self.showFPSSwitch.on forKey:@"showFPS"];
    } else if (sender == self.enableJITSwitch) {
        [defaults setBool:self.enableJITSwitch.on forKey:@"enableLightningJIT"];
    } else if (sender == self.vibrateSwitch) {
        [defaults setBool:self.vibrateSwitch.on forKey:@"vibrate"];
    } else if (sender == self.bumperSwitch) {
        [defaults setBool:self.bumperSwitch.on forKey:@"volumeBumper"];
    } else if (sender == self.dropboxSwitch) {//i'll use a better more foolproof method later. <- lol yeah right
        if ([defaults boolForKey:@"enableDropbox"] == false) {
            [[DBSession sharedSession] linkFromController:self];
        } else {
            [CHBgDropboxSync forceStopIfRunning];
            [CHBgDropboxSync clearLastSyncData];
            [[DBSession sharedSession] unlinkAll];
            SCLAlertView * alert = [[SCLAlertView alloc] init];
            [alert showInfo:self title:NSLocalizedString(@"UNLINKED", nil) subTitle:NSLocalizedString(@"UNLINKED_DETAIL", nil) closeButtonTitle:@"Okay!" duration:0.0];
            
            [defaults setBool:false forKey:@"enableDropbox"];
            self.accountLabel.text = NSLocalizedString(@"NOT_LINKED", nil);
        }
    } else if (sender == self.cellularSwitch) {
        [defaults setBool:self.cellularSwitch.on forKey:@"enableDropboxCellular"];
    }
}


- (void)appDidBecomeActive
{
    self.dropboxSwitch.on = [[NSUserDefaults standardUserDefaults] boolForKey:@"enableDropbox"];
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults boolForKey:@"enableDropbox"] == true) {
        self.accountLabel.text = NSLocalizedString(@"LINKED", nil);
    }
}

- (void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 4) { //Info
        switch (indexPath.row) {
          case 0: //Will Cobb
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/miniroo321"]];
            break;
        
          case 1: //Updates
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/iNDSapp"]];
            break;
        
          case 3: //DeSmuME
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"http://www.desmume.org/"]];
            break;
        
          case 4: //PMP174
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://github.com/pmp174"]];
            break;
        
          case 5: //Source
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://github.com/WilliamLCobb/iNDS"]];
            break;
        
          default:
            break;
        }
    }
}

#pragma mark - Hidden Settings

- (void)revealHiddenSettings:(UITapGestureRecognizer *)tapGestureRecognizer
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"revealHiddenSettings"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    [self.tableView reloadData];
}

#pragma mark - UITableView Data Source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    /*if ([[NSUserDefaults standardUserDefaults] boolForKey:@"revealHiddenSettings"]) {
        return 6 ;
    }*/
    
    return 5;
}

-(IBAction)back:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}
@end

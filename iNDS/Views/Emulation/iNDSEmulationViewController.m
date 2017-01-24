//
//  iNDSEmulationViewController.m
//  iNDS
//
//  Created by Will Cobb on 1/8/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationViewController.h"
#import "iNDSEmulationController.h"
#import "iNDSEmulationView.h"
#import "iNDSGamePadView.h"
#import "iNDSEmulationProfile.h"
#import "iNDSEmulationMenuView.h"


@interface iNDSEmulationViewController ()

@property iNDSEmulationController *emulationController;

@property (nonatomic, strong) iNDSEmulationView     *emulationView;
@property (nonatomic, strong) iNDSGamePadView       *controllerView;
@property (nonatomic, strong) iNDSEmulationMenuView *menuView;


@end

@implementation iNDSEmulationViewController

- (id)initWithEmulationController:(iNDSEmulationController *)controller {
    if (self = [super init]) {
        self.emulationView = controller.emulatorView;
        [self.view addSubview:self.emulationView];
        
        self.controllerView = [[iNDSGamePadView alloc] initWithFrame:self.view.bounds];
        [self.view addSubview:self.controllerView];
        
        self.menuView = [[iNDSEmulationMenuView alloc] initWithFrame:CGRectMake(0, self.view.frame.size.height - 200, self.view.frame.size.width, 200)];
        [self.view addSubview:self.menuView];
        
        [self setProfile:[[iNDSEmulationProfile alloc] initWithProfileName:@"Default"]];
        
        [self loadSettings];
    }
    return self;
}

- (void)loadSettings {
    self.controllerView.alpha = 0.5;
}

- (void)addBlueEffect {
//    UIVisualEffect *effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleDark];
//    UIVisualEffectView *blurEffectView = [[UIVisualEffectView alloc] initWithEffect:effect];
//    blurEffectView.frame = self.view.frame;
//    [blueEffectView addSubview:*your label*];
//    [self.view addSubview:self.blurEffectView];
}

- (void)setProfile:(iNDSEmulationProfile *)profile {
    [self.controllerView setProfile:profile];
    profile.mainScreen = self.emulationView.mainScreen;
    profile.touchScreen = self.emulationView.touchScreen;
    [self.view addSubview:profile.indicatorView];
    [profile ajustLayout];
}

- (void)showMenu {
    
}


@end

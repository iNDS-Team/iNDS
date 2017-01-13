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


@interface iNDSEmulationViewController ()

@property iNDSEmulationController *emulationController;

@property (nonatomic, strong) iNDSEmulationView *emulationView;
@property (nonatomic, strong) iNDSGamePadView    *controllerView;

@end

@implementation iNDSEmulationViewController

- (id)initWithEmulationController:(iNDSEmulationController *)controller {
    if (self = [super init]) {
        self.emulationView = controller.emulatorView;
        [self.view addSubview:self.emulationView];
        self.controllerView = [[iNDSGamePadView alloc] initWithFrame:self.view.bounds];
        [self.view addSubview:self.controllerView];
        [self setProfile:[[iNDSEmulationProfile alloc] initWithProfileName:@"Default"]];
    }
    return self;
}

- (void)setProfile:(iNDSEmulationProfile *)profile {
    [self.controllerView setProfile:profile];
    profile.mainScreen = self.emulationView.mainScreen;
    profile.touchScreen = self.emulationView.touchScreen;
    [self.view addSubview:profile.indicatorView];
    [profile ajustLayout];
}


@end

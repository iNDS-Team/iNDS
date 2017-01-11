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


@interface iNDSEmulationViewController ()

@property iNDSEmulationController *emulationController;

//@property (weak, nonatomic) UILabel *fpsLabel;
//
//@property (strong, nonatomic) UIView *controllerContainerView;
//@property (strong, nonatomic) UISlider *sizeSlider;
//
//@property (weak, nonatomic) iNDSDirectionalControl *directionalControl;
//@property (weak, nonatomic) iNDSButtonControl *buttonControl;
//@property (weak, nonatomic) UIButton *settingsButton;
//@property (weak, nonatomic) UIButton *startButton;
//@property (weak, nonatomic) UIButton *selectButton;
//@property (weak, nonatomic) UIButton *leftTrigger;
//@property (weak, nonatomic) UIButton *rightTrigger;

@property (nonatomic, strong) iNDSEmulationView *emulationView;

@end

@implementation iNDSEmulationViewController

- (id)initWithEmulationController:(iNDSEmulationController *)controller {
    if (self = [super init]) {
        self.emulationView = controller.emulatorView;
        [self.view addSubview:self.emulationView];
        
    }
    return self;
}


@end

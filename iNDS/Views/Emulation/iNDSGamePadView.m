//
//  iNDSGamePadView.m
//  iNDS
//
//  Created by Will Cobb on 1/10/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSGamePadView.h"
#import "iNDSButtonControl.h"
#import "iNDSDirectionalControl.h"

#import "indsemulationprofile.h"

@interface iNDSGamePadView ()

@property (strong, nonatomic) UILabel *fpsLabel;
@property (strong, nonatomic) UIButton *settingsButton;
@property (strong, nonatomic) UIButton *startButton;
@property (strong, nonatomic) UIButton *selectButton;
@property (strong, nonatomic) UIButton *leftTrigger;
@property (strong, nonatomic) UIButton *rightTrigger;
@property (strong, nonatomic) iNDSDirectionalControl *directionalControl;
@property (strong, nonatomic) iNDSButtonControl *buttonControl;
@property (strong, nonatomic) UISlider *sizeSlider;

@end

@implementation iNDSGamePadView

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        [self initializeViews];
    }
    return self;
}

- (void)initializeViews {
    self.directionalControl = [[iNDSDirectionalControl alloc] init];
    [self addSubview:self.directionalControl];
    
    self.buttonControl = [[iNDSButtonControl alloc] init];
    [self addSubview:self.buttonControl];
    
    self.settingsButton = [[UIButton alloc] init];
    [self.settingsButton setImage:[UIImage imageNamed:@"HideEmulator"] forState:UIControlStateNormal];
    [self addSubview:self.settingsButton];
    
    self.startButton = [[UIButton alloc] init];
    [self.startButton setImage:[UIImage imageNamed:@"Start"] forState:UIControlStateNormal];
    [self addSubview:self.startButton];
    
    self.selectButton = [[UIButton alloc] init];
    [self.selectButton setImage:[UIImage imageNamed:@"Select"] forState:UIControlStateNormal];
    [self addSubview:self.selectButton];
    
    self.leftTrigger = [[UIButton alloc] init];
    [self.leftTrigger setImage:[UIImage imageNamed:@"LTrigger"] forState:UIControlStateNormal];
    [self addSubview:self.leftTrigger];
    
    self.rightTrigger = [[UIButton alloc] init];
    [self.rightTrigger setImage:[UIImage imageNamed:@"RTrigger"] forState:UIControlStateNormal];
    [self addSubview:self.rightTrigger];
    
    self.fpsLabel = [[UILabel alloc] init];
    [self addSubview:self.fpsLabel];
    
    self.sizeSlider = [[UISlider alloc] init];
    [self addSubview:self.sizeSlider];
}

- (void)setProfile:(iNDSEmulationProfile *)profile {
    _profile = profile;
    self.profile.directionalControl = self.directionalControl;
    self.profile.buttonControl = self.buttonControl;
    self.profile.settingsButton = self.settingsButton;
    self.profile.startButton = self.startButton;
    self.profile.selectButton = self.selectButton;
    self.profile.leftTrigger = self.leftTrigger;
    self.profile.rightTrigger = self.rightTrigger;
    self.profile.fpsLabel = self.fpsLabel;
    self.profile.sizeSlider = self.sizeSlider;
    [self.sizeSlider addTarget:self.profile action:@selector(sizeChanged:) forControlEvents:UIControlEventValueChanged];
}

@end

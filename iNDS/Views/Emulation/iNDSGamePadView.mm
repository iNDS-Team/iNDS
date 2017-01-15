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
#import "iNDSEmulationProfile.h"
#import "emu.h"
#import "UITapticEngine.h"
#import "UIDevice+Private.h"
#import <AudioToolbox/AudioToolbox.h>

@interface iNDSGamePadView () {
    iNDSButtonControlButton _previousButtons;
    iNDSDirectionalControlDirection _previousDirection;
}

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
    [self.directionalControl addTarget:self action:@selector(pressedDPad:) forControlEvents:UIControlEventValueChanged];
    [self addSubview:self.directionalControl];
    
    self.buttonControl = [[iNDSButtonControl alloc] init];
    [self.buttonControl addTarget:self action:@selector(pressedABXY:) forControlEvents:UIControlEventValueChanged];
    [self addSubview:self.buttonControl];
    
    self.settingsButton = [[UIButton alloc] init];
    [self.settingsButton setImage:[UIImage imageNamed:@"HideEmulator"] forState:UIControlStateNormal];
    [self addSubview:self.settingsButton];
    
    self.startButton = [[UIButton alloc] init];
    self.startButton.tag = 5;
    [self.startButton addTarget:self action:@selector(onButtonDown:) forControlEvents:UIControlEventTouchDown];
    [self.startButton addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpInside];
    [self.startButton addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpOutside];
    [self.startButton setImage:[UIImage imageNamed:@"Start"] forState:UIControlStateNormal];
    [self addSubview:self.startButton];
    
    self.selectButton = [[UIButton alloc] init];
    self.selectButton.tag = 4;
    [self.selectButton addTarget:self action:@selector(onButtonDown:) forControlEvents:UIControlEventTouchDown];
    [self.selectButton addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpInside];
    [self.selectButton addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpOutside];
    [self.selectButton setImage:[UIImage imageNamed:@"Select"] forState:UIControlStateNormal];
    [self addSubview:self.selectButton];
    
    self.leftTrigger = [[UIButton alloc] init];
    self.leftTrigger.tag = 10;
    [self.leftTrigger addTarget:self action:@selector(onButtonDown:) forControlEvents:UIControlEventTouchDown];
    [self.leftTrigger addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpInside];
    [self.leftTrigger addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpOutside];
    [self.leftTrigger setImage:[UIImage imageNamed:@"LTrigger"] forState:UIControlStateNormal];
    [self addSubview:self.leftTrigger];
    
    self.rightTrigger = [[UIButton alloc] init];
    self.rightTrigger.tag = 11;
    [self.rightTrigger addTarget:self action:@selector(onButtonDown:) forControlEvents:UIControlEventTouchDown];
    [self.rightTrigger addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpInside];
    [self.rightTrigger addTarget:self action:@selector(onButtonUp:) forControlEvents:UIControlEventTouchUpOutside];
    [self.rightTrigger setImage:[UIImage imageNamed:@"RTrigger"] forState:UIControlStateNormal];
    [self addSubview:self.rightTrigger];
    
    self.fpsLabel = [[UILabel alloc] init];
    [self addSubview:self.fpsLabel];
    
    self.sizeSlider = [[UISlider alloc] init];
    self.sizeSlider.hidden = YES;
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

- (void)layoutSubviews {
    self.sizeSlider.frame = CGRectMake(10, self.frame.size.height - 35, self.frame.size.width - 20, self.sizeSlider.frame.size.height);
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    for (UIView *subview in self.subviews) {
        if (CGRectContainsPoint(subview.frame, point)) {
            return YES;
        }
    }
    return NO;
}

- (IBAction)pressedDPad:(iNDSDirectionalControl *)sender
{
    iNDSDirectionalControlDirection state = sender.direction;
    
    EMU_setDPad(state & iNDSDirectionalControlDirectionUp, state & iNDSDirectionalControlDirectionDown, state & iNDSDirectionalControlDirectionLeft, state & iNDSDirectionalControlDirectionRight);
    
    _previousDirection = state;
}

- (IBAction)pressedABXY:(iNDSButtonControl *)sender
{
    iNDSButtonControlButton state = sender.selectedButtons;
    if (state != _previousButtons && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
        {
            [self vibrate];
        }
    }
    
    EMU_setABXY(state & iNDSButtonControlButtonA, state & iNDSButtonControlButtonB, state & iNDSButtonControlButtonX, state & iNDSButtonControlButtonY);
    
    _previousButtons = state;
}

- (IBAction)onButtonUp:(UIControl*)sender
{
    EMU_buttonUp((BUTTON_ID)sender.tag);
}

- (IBAction)onButtonDown:(UIControl*)sender
{
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
    {
        [self vibrate];
    }
    EMU_buttonDown((BUTTON_ID)sender.tag);
}

FOUNDATION_EXTERN void AudioServicesStopSystemSound(int);
FOUNDATION_EXTERN void AudioServicesPlaySystemSoundWithVibration(unsigned long, objc_object*, NSDictionary*);

- (void)vibrate
{
    // If force touch is avaliable we can assume taptic vibration is too
    if ([[self traitCollection] respondsToSelector:@selector(forceTouchCapability)] && [[self traitCollection] forceTouchCapability] == UIForceTouchCapabilityAvailable) {
        [[[UIDevice currentDevice] tapticEngine] actuateFeedback:UITapticEngineFeedbackPeek];
    } else {
        AudioServicesStopSystemSound(kSystemSoundID_Vibrate);
        
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        NSArray *pattern = @[@YES, @20, @NO, @1];
        
        dictionary[@"VibePattern"] = pattern;
        dictionary[@"Intensity"] = @1;
        
        AudioServicesPlaySystemSoundWithVibration(kSystemSoundID_Vibrate, nil, dictionary);
    }
}

@end

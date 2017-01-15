//
//  ButtonControl.m
//  iNDS
//
//  Created by Riley Testut on 7/5/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSButtonControl.h"

@interface iNDSDirectionalControl ()

@property (strong, nonatomic) UIImageView *backgroundImageView;

- (iNDSDirectionalControlDirection)directionForTouch:(UITouch *)touch;

@end

@interface iNDSButtonControl () {
    iNDSButtonControlButton _selectedButtons;
    UITouch *touches[4];
}

@property (readwrite, nonatomic) iNDSButtonControlButton selectedButtons;

@end

@implementation iNDSButtonControl

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        self.backgroundImageView.image = [UIImage imageNamed:@"ABXYPad"];
        self.multipleTouchEnabled = YES;
    }
    return self;
}

- (id)init
{
    self = [super init];
    if (self) {
        self.backgroundImageView.image = [UIImage imageNamed:@"ABXYPad"];
        self.multipleTouchEnabled = YES;
    }
    return self;
}



- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    for (int i = 0; i < 4; i++) {
        if (!touches[i]) {
            touches[i] = touch;
            break;
        }
    }
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    return YES;
}

- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    return YES;
}

- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    for (int i = 0; i < 4; i++) {
        if (touch == touches[i]) {
            touches[i] = NULL;
            break;
        }
    }
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

- (iNDSButtonControlButton)selectedButtons {
    iNDSButtonControlButton buttons = 0;
    for (int i = 0; i < 4; i++) {
        if (touches[i]) {
            buttons |= [self directionForTouch:touches[i]];
        }
    }
    return buttons;
}

@end

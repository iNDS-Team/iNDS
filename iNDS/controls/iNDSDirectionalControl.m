//
//  iNDSDirectionalControl.m
//  iNDS
//
//  Created by iNDS
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSDirectionalControl.h"

@interface iNDSDirectionalControl()

@property (readwrite, nonatomic) iNDSDirectionalControlDirection direction;
@property (assign, nonatomic) CGSize deadZone; // dead zone in the middle of the control
@property (strong, nonatomic) UIImageView *backgroundImageView;
@property (assign, nonatomic) CGRect deadZoneRect;
@property (strong, nonatomic) UIImageView *buttonImageView;

@end

@implementation iNDSDirectionalControl

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        // Initialization code
        _backgroundImageView = [[UIImageView alloc] initWithFrame:self.bounds];
        _backgroundImageView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
        [self addSubview:_backgroundImageView];
        
        _buttonImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 56, 56)];
        _buttonImageView.image = [UIImage imageNamed:@"JoystickButton"];
        _buttonImageView.center = CGPointMake(self.frame.size.width/2, self.frame.size.height/2);
        [self addSubview:_buttonImageView];
        
        self.deadZone = CGSizeMake(self.frame.size.width/3, self.frame.size.height/3);
        
        [self setStyle:iNDSDirectionalControlStyleDPad];
    }
    return self;
}
- (void) frameUpdated
{
    _buttonImageView.center = CGPointMake(self.frame.size.width/2, self.frame.size.height/2);
    self.deadZone = CGSizeMake(self.frame.size.width/3, self.frame.size.height/3);
    
    _buttonImageView.frame = CGRectMake(0, 0, _backgroundImageView.frame.size.width/2, _backgroundImageView.frame.size.width/2);
}

- (iNDSDirectionalControlDirection)directionForTouch:(UITouch *)touch
{
    // convert coords to based on center of control
    CGPoint loc = [touch locationInView:self];
    if (!CGRectContainsPoint(self.bounds, loc)) return 0;
    iNDSDirectionalControlDirection direction = 0;
    
    if (loc.x > (self.bounds.size.width + self.deadZone.width)/2) direction |= iNDSDirectionalControlDirectionRight;
    else if (loc.x < (self.bounds.size.width - self.deadZone.width)/2) direction |= iNDSDirectionalControlDirectionLeft;
    if (loc.y > (self.bounds.size.height + self.deadZone.height)/2) direction |= iNDSDirectionalControlDirectionDown;
    else if (loc.y < (self.bounds.size.height - self.deadZone.height)/2) direction |= iNDSDirectionalControlDirectionUp;
    
    return direction;
}

- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    self.direction = [self directionForTouch:touch];
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    if (self.style == iNDSDirectionalControlStyleJoystick) {
        CGPoint loc = [touch locationInView:self];
        self.deadZoneRect = CGRectMake((self.bounds.size.width - self.deadZone.width)/2, (self.bounds.size.height - self.deadZone.height)/2, self.deadZone.width, self.deadZone.height);
        if (!CGRectContainsPoint(self.deadZoneRect, loc)) return NO;
        self.buttonImageView.center = loc;
    }
    
    return YES;
}

- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    self.direction = [self directionForTouch:touch];
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    if (self.style == iNDSDirectionalControlStyleJoystick) {
        if (![super continueTrackingWithTouch:touch withEvent:event]) return NO;
        
        // keep button inside
        CGPoint loc = [touch locationInView:self];
        loc.x -= self.bounds.size.width/2;
        loc.y -= self.bounds.size.height/2;
        double radius = sqrt(loc.x*loc.x+loc.y*loc.y);
        double maxRadius = self.bounds.size.width * 0.45;
        if (radius > maxRadius) {
            double angle = atan(loc.y/loc.x);
            if (loc.x < 0) angle += M_PI;
            radius = maxRadius;
            loc.x = radius * cos(angle);
            loc.y = radius * sin(angle);
        }
        loc.x += self.bounds.size.width/2;
        loc.y += self.bounds.size.height/2;
        self.buttonImageView.center = loc;
        
    }
    return YES;
}

- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    self.direction = 0;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    if (self.style == iNDSDirectionalControlStyleJoystick) {
        self.buttonImageView.center = self.backgroundImageView.center;
    }
}

#pragma mark - Getters/Setters

- (void)setStyle:(iNDSDirectionalControlStyle)style {
    switch (style) {
        case iNDSDirectionalControlStyleDPad: {
            self.buttonImageView.hidden = YES;
            self.backgroundImageView.image = [UIImage imageNamed:@"DPad"];
            break;
        }
            
        case iNDSDirectionalControlStyleJoystick: {
            self.buttonImageView.hidden = NO;
            self.backgroundImageView.image = [UIImage imageNamed:@"JoystickBackground"];
            break;
        }
    }
    
    _style = style;
}

@end

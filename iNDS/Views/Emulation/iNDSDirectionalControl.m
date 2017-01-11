//
//  iNDSDirectionalControl.m
//  iNDS
//
//  Created by Will Cobb
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSDirectionalControl.h"

@interface iNDSDirectionalControl() {
    CALayer *buttonImage;
    CFTimeInterval lastMove;
}

@property (readwrite, nonatomic) iNDSDirectionalControlDirection direction;
@property (assign, nonatomic) CGSize deadZone; // dead zone in the middle of the control
@property (strong, nonatomic) UIImageView *backgroundImageView;
@property (assign, nonatomic) CGRect deadZoneRect;

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
        
        buttonImage = [[CALayer alloc] init];
        buttonImage.frame = CGRectMake(0, 0, self.frame.size.width/2.2, self.frame.size.width/2.2);
        buttonImage.contents = (id)[UIImage imageNamed:@"JoystickButton"].CGImage;
        buttonImage.anchorPoint = CGPointMake(0.5, 0.5);
        buttonImage.actions = @{@"position": [NSNull null]};
        [self.layer addSublayer:buttonImage];
        
        self.deadZone = CGSizeMake(self.frame.size.width/3, self.frame.size.height/3);
        self.direction = iNDSDirectionalControlDirectionDown;
        
        
        [self setStyle:iNDSDirectionalControlStyleDPad];
    }
    return self;
}
- (void) frameUpdated
{
    self.deadZone = CGSizeMake(self.frame.size.width/3, self.frame.size.height/3);
    self.deadZoneRect = CGRectMake((self.bounds.size.width - self.deadZone.width)/2, (self.bounds.size.height - self.deadZone.height)/2, self.deadZone.width, self.deadZone.height);
    buttonImage.frame = CGRectMake(0, 0, self.frame.size.width/2.2, self.frame.size.width/2.2);
    buttonImage.position = self.backgroundImageView.center;
}



- (void) layoutSubviews
{
    [super layoutSubviews];
    [self frameUpdated];
}


- (iNDSDirectionalControlDirection)directionForTouch:(UITouch *)touch
{
    // convert coords to based on center of control
    CGPoint loc = [touch locationInView:self];
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
        buttonImage.position = loc;
        lastMove = CACurrentMediaTime();
    }
    return YES;
}

- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    self.direction = [self directionForTouch:touch];
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    if (self.style == iNDSDirectionalControlStyleJoystick) {
        // keep button inside
        CGPoint loc = [touch locationInView:self];
        loc.x -= self.bounds.size.width/2;
        loc.y -= self.bounds.size.height/2;
        double radius = sqrt(loc.x*loc.x+loc.y*loc.y);
        double maxRadius = self.bounds.size.width * 0.45;
        if (radius > maxRadius) {
            double angle = atan(loc.y/loc.x);
            if (loc.x < 0) angle += M_PI;
            loc.x = maxRadius * cos(angle);
            loc.y = maxRadius * sin(angle);
        }
        loc.x += self.bounds.size.width/2;
        loc.y += self.bounds.size.height/2;
        
        // increasing this value reduces refresh rate and greatly increases performance.
        // But it makes the button look jittery in comparison the other UI elements
        if (CACurrentMediaTime() - lastMove > 0.033) {
            buttonImage.position = loc;
            lastMove = CACurrentMediaTime();
        }
    }
    return YES;
}

- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    self.direction = 0;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    if (self.style == iNDSDirectionalControlStyleJoystick) {
        buttonImage.position = CGPointMake(self.bounds.size.width/2, self.bounds.size.height/2);
    }
}

#pragma mark - Getters/Setters

- (void)setStyle:(iNDSDirectionalControlStyle)style {
    switch (style) {
        case iNDSDirectionalControlStyleDPad: {
            buttonImage.hidden = YES;
            self.backgroundImageView.image = [UIImage imageNamed:@"DPad"];
            break;
        }
            
        case iNDSDirectionalControlStyleJoystick: {
            buttonImage.hidden = NO;
            self.backgroundImageView.image = [UIImage imageNamed:@"JoystickBackground"];
            break;
        }
    }
    _style = style;
}

@end

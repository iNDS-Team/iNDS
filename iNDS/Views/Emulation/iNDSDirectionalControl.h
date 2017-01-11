//
//  iNDSDirectionalControl.h
//  iNDS
//
//  Created by Will Cobb
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, iNDSDirectionalControlDirection) {
    iNDSDirectionalControlDirectionUp     = 1 << 0,
    iNDSDirectionalControlDirectionDown   = 1 << 1,
    iNDSDirectionalControlDirectionLeft   = 1 << 2,
    iNDSDirectionalControlDirectionRight  = 1 << 3,
};

typedef NS_ENUM(NSInteger, iNDSDirectionalControlStyle) {
    iNDSDirectionalControlStyleDPad = 0,
    iNDSDirectionalControlStyleJoystick = 1,
};

@interface iNDSDirectionalControl : UIControl

@property (readonly, nonatomic) iNDSDirectionalControlDirection direction;
@property (assign, nonatomic) iNDSDirectionalControlStyle style;

- (void) frameUpdated;

@end

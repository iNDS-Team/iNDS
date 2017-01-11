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

@end

@interface iNDSButtonControl ()

@property (readwrite, nonatomic) iNDSButtonControlButton selectedButtons;

@end

@implementation iNDSButtonControl

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        // Initialization code
        
        self.backgroundImageView.image = [UIImage imageNamed:@"ABXYPad"];
    }
    return self;
}

- (iNDSButtonControlButton)selectedButtons {
    return (iNDSButtonControlButton)self.direction;
}

@end

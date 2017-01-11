//
//  ButtonControl.h
//  iNDS
//
//  Created by iNDS on 7/5/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSDirectionalControl.h"

// This class really doesn't do much. It's basically here to make the code easier to read, but also in case of future expansion.

// Below are identical to the superclass variants, just renamed for clarity
typedef NS_ENUM(NSInteger, iNDSButtonControlButton) {
    iNDSButtonControlButtonX     = 1 << 0,
    iNDSButtonControlButtonB     = 1 << 1,
    iNDSButtonControlButtonY     = 1 << 2,
    iNDSButtonControlButtonA     = 1 << 3,
};

@interface iNDSButtonControl : iNDSDirectionalControl

@property (readonly, nonatomic) iNDSButtonControlButton selectedButtons;

@end

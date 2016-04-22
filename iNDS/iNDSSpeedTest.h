//
//  iNDSSpeedTest.h
//  iNDS
//
//  Created by Will Cobb on 4/21/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>

enum {
    NONE,
    HQ2X,
    _2XSAI,
    SUPER2XSAI,
    SUPEREAGLE,
    SCANLINE,
    BILINEAR,
    NEAREST2X,
    HQ2XS,
    LQ2X,
    LQ2XS,
    EPX,
    NEARESTPLUS1POINT5,
    NEAREST1POINT5,
    EPXPLUS,
    EPX1POINT5,
    EPXPLUS1POINT5,
    HQ4X,
    BRZ2x,
    BRZ3x,
    BRZ4x,
    BRZ5x,
    
    NUM_FILTERS,
};

@interface iNDSSpeedTest : NSObject

+ (NSArray *)filterTimesForFilters:(NSArray *)filters;

@end

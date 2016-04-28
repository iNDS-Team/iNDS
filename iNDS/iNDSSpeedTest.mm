//
//  iNDSSpeedTest.m
//  iNDS
//
//  Created by Will Cobb on 4/21/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSSpeedTest.h"
#import <UIKit/UIKit.h>

#include "types.h"
#include "video.h"


u32 speedBuffer[16*256*192*2];
u32 filteredSpeedBuffer[16*256*192*2];

@implementation iNDSSpeedTest

+(CFTimeInterval)averageCoreSpeed
{
    return [[NSUserDefaults standardUserDefaults] floatForKey:@"coreTime"];
}

+ (NSArray *)filterTimesForFilters:(NSArray *)filters
{
    NSMutableArray *times = [NSMutableArray array];
    
    SSurface src;
    SSurface dst;
    src.Height = 384;
    src.Width = 256;
    src.Pitch = 512;
    src.Surface = (u8*)speedBuffer;
    
    for (NSNumber *filter in filters) {
        int width, height;
        switch([filter integerValue]) {
            case NONE:
                width = 256;
                height = 384;
                break;
            case EPX1POINT5:
            case EPXPLUS1POINT5:
            case NEAREST1POINT5:
            case NEARESTPLUS1POINT5:
                width = 256*3/2;
                height = 384*3/2;
                break;
            case BRZ3x:
                width = 256*3;
                height = 384*3;
                break;
            case HQ4X:
            case BRZ4x:
                width = 256*4;
                height = 384*4;
                break;
            case BRZ5x:
                width = 256*5;
                height = 384*5;
                break;
            default:
                width = 256*2;
                height = 384*2;
                break;
        }
        
        dst.Height = height;
        dst.Width = width;
        dst.Pitch = width*2;
        dst.Surface = (u8*)filteredSpeedBuffer;
        
        CFTimeInterval now = CACurrentMediaTime() ;
        for (int i = 0; i < 100; i++) {
            switch([filter integerValue])
            {
                case NONE:
                    break;
                case LQ2X:
                    RenderLQ2X(src, dst);
                    break;
                case LQ2XS:
                    RenderLQ2XS(src, dst);
                    break;
                case HQ2X:
                    RenderHQ2X(src, dst);
                    break;
                case HQ4X:
                    RenderHQ4X(src, dst);
                    break;
                case HQ2XS:
                    RenderHQ2XS(src, dst);
                    break;
                case _2XSAI:
                    Render2xSaI (src, dst);
                    break;
                case SUPER2XSAI:
                    RenderSuper2xSaI (src, dst);
                    break;
                case SUPEREAGLE:
                    RenderSuperEagle (src, dst);
                    break;
                case SCANLINE:
                    RenderScanline(src, dst);
                    break;
                case BILINEAR:
                    RenderBilinear(src, dst);
                    break;
                case NEAREST2X:
                    RenderNearest2X(src,dst);
                    break;
                case EPX:
                    RenderEPX(src,dst);
                    break;
                case EPXPLUS:
                    RenderEPXPlus(src,dst);
                    break;
                case EPX1POINT5:
                    RenderEPX_1Point5x(src,dst);
                    break;
                case EPXPLUS1POINT5:
                    RenderEPXPlus_1Point5x(src,dst);
                    break;
                case NEAREST1POINT5:
                    RenderNearest_1Point5x(src,dst);
                    break;
                case NEARESTPLUS1POINT5:
                    RenderNearestPlus_1Point5x(src,dst);
                    break;
            }
        }
        [times addObject:@((CACurrentMediaTime() - now)/100)];
    }
    return times;
}

@end

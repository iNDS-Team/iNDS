//
//  iNDSEmulationController.h
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <UIKit/UIKit.h>

@class iNDSROM;
@class iNDSEmulationView;
@class iNDSGamePadView;

@interface iNDSEmulationController : NSObject

@property (nonatomic, readonly) iNDSEmulationView *emulatorView;
@property (nonatomic, readonly) CGFloat fps;

@property (nonatomic, assign) NSInteger speed;

- (id)initWithRom:(iNDSROM *)rom;

@end

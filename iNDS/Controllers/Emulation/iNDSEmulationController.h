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

@interface iNDSEmulationController : NSObject

@property (nonatomic, readonly) iNDSEmulationView *emulatorView;

- (id)initWithRom:(iNDSROM *)rom;

@end

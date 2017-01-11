//
//  RomCollectionController.h
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface iNDSROMCollectionController : NSObject

@property (nonatomic, strong, readonly) NSMutableArray *roms;

- (void)updateRoms;

@end

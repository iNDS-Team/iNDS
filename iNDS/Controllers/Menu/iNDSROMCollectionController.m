//
//  RomCollectionController.m
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSROMCollectionController.h"
#import "iNDSROM.h"
#import "iNDSSettings.h"

@interface iNDSROMCollectionController ()

@property (nonatomic, weak) iNDSSettings *settings;

@end

@implementation iNDSROMCollectionController

- (id)init {
    if (self = [super init]) {
        self.settings = [iNDSSettings sharedInstance];
    }
    return self;
}

- (void)updateRoms {
    _roms = [[iNDSROM ROMsAtPath:self.settings.documentsPath saveStateDirectoryPath:self.settings.batteryPath] mutableCopy];
    NSString *romsPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"ROMS"];
    [_roms addObjectsFromArray:[iNDSROM ROMsAtPath:romsPath saveStateDirectoryPath:self.settings.batteryPath]];
}

@end

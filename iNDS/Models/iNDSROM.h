//
//  iNDSRom.h
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface iNDSROM : NSObject

@property (strong, nonatomic) NSString *path;
@property (nonatomic, readonly) NSString *title;
@property (nonatomic, readonly) NSString *rawTitle;
@property (nonatomic, readonly) NSString *gameTitle;
@property (nonatomic, readonly) UIImage *icon;
@property (nonatomic, readonly) NSInteger numberOfSaveStates;
@property (strong, nonatomic) NSString *pathForSavedStates;
@property (nonatomic, readonly) BOOL hasPauseState;

+ (int)preferredLanguage; // returns a NDS_FW_LANG_ constant
+ (NSArray *)ROMsAtPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;
+ (iNDSROM*)ROMWithPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;

- (iNDSROM *)initWithPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;
- (NSString *)pathForSaveStateWithName:(NSString*)name;
- (NSString *)pathForSaveStateAtIndex:(NSInteger)idx;
- (NSString *)nameOfSaveStateAtIndex:(NSInteger)idx;
- (NSString *)nameOfSaveStateAtPath:(NSString*)path;
- (NSDate *)dateOfSaveStateAtIndex:(NSInteger)idx;
- (BOOL)deleteSaveStateAtIndex:(NSInteger)idx;
- (BOOL)deleteSaveStateWithName:(NSString *)name;
- (void)reloadSaveStates;
- (NSArray *)saveStates;

@end

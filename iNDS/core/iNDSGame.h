//
//  iNDSGame.h
//  iNDS
//
//  Created by Zydeco on 16/7/2013.
//  Copyright (c) 2013 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSString * const iNDSGameSaveStatesChangedNotification;

@interface iNDSGame : NSObject

@property (strong, nonatomic) NSString *path;
@property (nonatomic, readonly) NSString *title;
@property (nonatomic, readonly) NSString *rawTitle;
@property (nonatomic, readonly) NSString *gameTitle;
@property (nonatomic, readonly) NSString *origTitle;
@property (nonatomic, readonly) UIImage *icon;
@property (nonatomic, readonly) NSInteger numberOfSaveStates;
@property (strong, nonatomic) NSString *pathForSavedStates;
@property (nonatomic, readonly) BOOL hasPauseState;
@property (nonatomic, readonly) NSDictionary *altTitles;

+ (int)preferredLanguage; // returns a NDS_FW_LANG_ constant
+ (NSArray*)gamesAtPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;
+ (iNDSGame*)gameWithPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;
+ (iNDSGame*)gameWithName:(NSString *)name;
- (iNDSGame*)initWithPath:(NSString*)path saveStateDirectoryPath:(NSString*)saveStatePath;
- (NSString*)pathForSaveStateWithName:(NSString*)name;
- (NSString*)pathForSaveStateAtIndex:(NSInteger)idx;
- (NSString*)nameOfSaveStateAtIndex:(NSInteger)idx;
- (NSString*)nameOfSaveStateAtPath:(NSString*)path;
- (NSDate*)dateOfSaveStateAtIndex:(NSInteger)idx;
- (BOOL)deleteSaveStateAtIndex:(NSInteger)idx;
- (BOOL)deleteSaveStateWithName:(NSString *)name;
- (void)reloadSaveStates;
- (NSArray*)saveStates;
- (void) setAltTitle:(NSString *)altTitle;
- (NSString *) imageURL;
- (BOOL) isEqual:(id)object;
- (NSUInteger) hash;

@end

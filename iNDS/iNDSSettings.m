//
//  iNDSSettingsController.m
//  iNDS
//
//  Created by Will Cobb on 1/7/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSSettings.h"

@implementation iNDSSettings

+ (instancetype)sharedInstance {
    static dispatch_once_t p = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&p, ^{
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

- (void)_initDefaults {
//    if (![[NSUserDefaults standardUserDefaults] stringForKey:@"documentsPath"]) {
//        [[NSUserDefaults standardUserDefaults] setObject:[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]
//                                                  forKey:@"documentsPath"];
//    }
    
    // Create Folders
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    [fileManager createDirectoryAtPath:self.batteryPath withIntermediateDirectories:YES attributes:nil error:NULL];
}

- (NSString *)documentsPath {
    return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
}

- (NSString *)batteryPath {
    return [self.documentsPath stringByAppendingPathComponent:@"Battery"];
}

@end

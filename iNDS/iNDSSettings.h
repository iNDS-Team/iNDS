//
//  iNDSSettingsController.h
//  iNDS
//
//  Created by Will Cobb on 1/7/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//
// https://github.com/futuretap/InAppSettingsKit

#import <Foundation/Foundation.h>

@interface iNDSSettings : NSObject

@property (nonatomic, strong, readonly) NSString *documentsPath;
@property (nonatomic, strong, readonly) NSString *batteryPath;

+(instancetype)sharedInstance;

@end

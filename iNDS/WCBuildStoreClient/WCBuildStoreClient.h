//
//  WCBuildStoreClient.h
//  iNDS
//
//  Created by Will Cobb on 5/5/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

// Not completed due to the Build Store's requirement of certificate installing

#import <Foundation/Foundation.h>
#import "WCBuildStoreAuthenticateViewController.h"

@interface WCBuildStoreClient : NSObject

@property (nonatomic, getter=getLinked)BOOL linked;

+ (WCBuildStoreClient *)sharedInstance;

- (void)linkFromController:(UIViewController *)controller;
- (void)unlink;
- (void)checkForUpdates;
@end

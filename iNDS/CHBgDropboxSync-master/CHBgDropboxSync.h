//
//  CHBgDropboxSync.h
//  Passwords
//
//  Created by Chris Hulbert on 4/03/12.
//

#import <Foundation/Foundation.h>
//#import <DropboxSDK/DBRestClient.h>
#import <ObjectiveDropboxOfficial/ObjectiveDropboxOfficial.h>

@interface CHBgDropboxSync : NSObject<UIAlertViewDelegate>

+ (void)start;
+ (void)forceStopIfRunning;
+ (void)clearLastSyncData;
- (void)grabMetadata;

@end

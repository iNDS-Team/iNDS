//
//  iNDSMasterViewController.h
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <SDWebImage/SDWebImageManager.h>
//#import <DropboxSDK/DropboxSDK.h>

@interface iNDSROMTableViewController : UITableViewController <UIAlertViewDelegate, SDWebImageManagerDelegate>
{
    NSArray *games;
}

- (void)reloadGames:(id)sender;
- (void) userRequestedToPlayROM:(NSNotification *) notification;

@end

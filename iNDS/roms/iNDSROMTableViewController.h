//
//  iNDSMasterViewController.h
//  iNDS
//
//  Created by iNDS on 6/9/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <DropboxSDK/DropboxSDK.h>

@interface iNDSROMTableViewController : UITableViewController <UIAlertViewDelegate>
{
    NSArray *games;
    
    IBOutlet UINavigationItem *romListTitle;
}

- (void)reloadGames:(id)sender;

@end

//
//  iNDSAboutViewController.h
//  iNDS
//
//  Created by Developer on 7/8/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface iNDSAboutViewController : UITableViewController {
    BOOL _canTweet;
    IBOutlet UINavigationItem *aboutTitle;    
    IBOutlet UIBarButtonItem *tweetButton;
    IBOutlet UILabel *versionLabel;
    IBOutlet UILabel *desmumeVersion;
}
- (IBAction)sendTweet:(id)sender;

@end

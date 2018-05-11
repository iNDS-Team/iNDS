//
//  iNDSDropboxTableViewController.h
//  iNDS
//
//  Created by Will Cobb on 4/22/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <ObjectiveDropboxOfficial/ObjectiveDropboxOfficial.h>

@interface iNDSDropboxTableViewController : UITableViewController

- (void)loadedAccountInfo:(DBUSERSFullAccount *)info;

@end

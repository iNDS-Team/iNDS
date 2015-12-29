//
//  iNDSGameTableView.h
//  iNDS
//
//  Created by Will Cobb on 11/4/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

@class iNDSGame;
@interface iNDSGameTableView : UITableViewController

@property (atomic, strong) iNDSGame * game;

@end

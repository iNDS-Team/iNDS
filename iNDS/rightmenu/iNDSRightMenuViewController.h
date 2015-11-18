//
//  iNDSRightMenuViewController.h
//  iNDS
//
//  Created by David Chavez on 7/15/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SASlideMenuRootViewController.h"
#import "iNDSGame.h"

@interface iNDSRightMenuViewController : UITableViewController

@property (weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (strong, nonatomic) iNDSGame *game;

@end

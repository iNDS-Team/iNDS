//
//  iNDSBugReportRomTableViewController.h
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSROMTableViewController.h"
#import "iNDSBugReportTableViewController.h"

@interface iNDSBugReportRomTableViewController : iNDSROMTableViewController

@property id <iNDSBugReportSelectionDelegate> delegate;

@end

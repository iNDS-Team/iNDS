//
//  iNDSBugReportSaveStateTableViewController.h
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSGameTableView.h"
#import "iNDSBugReportTableViewController.h"

@interface iNDSBugReportSaveStateTableViewController : iNDSGameTableView

@property id <iNDSBugReportSelectionDelegate> delegate;

@end

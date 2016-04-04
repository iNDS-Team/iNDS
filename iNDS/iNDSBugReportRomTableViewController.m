//
//  iNDSBugReportRomTableViewController.m
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSBugReportRomTableViewController.h"
#import "iNDSBugReportSaveStateTableViewController.h"
#import "iNDSGame.h"
@implementation iNDSBugReportRomTableViewController

- (void)viewDidLoad
{
    self.navigationItem.title = NSLocalizedString(@"ROM_LIST", nil);
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}


- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return NO;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 0) {
        iNDSGame *game = games[indexPath.row];
        [tableView deselectRowAtIndexPath:indexPath animated:YES];
        [self.delegate setGame:game];
        [self.navigationController popToRootViewControllerAnimated:YES];
    } else {
        [tableView deselectRowAtIndexPath:indexPath animated:YES];
    }
}
@end

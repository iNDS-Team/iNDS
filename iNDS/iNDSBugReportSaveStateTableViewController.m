//
//  iNDSBugReportSaveStateTableViewController.m
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "iNDSBugReportSaveStateTableViewController.h"
#import "iNDSGame.h"

@implementation iNDSBugReportSaveStateTableViewController

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return NO;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    if (section == 0)
        return 0;
    return self.game.saveStates.count;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self.delegate setSaveStatePath:[self.game pathForSaveStateAtIndex:indexPath.row]];
    [self.navigationController popToRootViewControllerAnimated:YES];
}

@end

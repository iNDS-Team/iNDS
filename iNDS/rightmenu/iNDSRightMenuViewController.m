//
//  iNDSRightMenuViewController.m
//  iNDS
//
//  Created by David Chavez on 7/15/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSRightMenuViewController.h"
#import "AppDelegate.h"

@interface iNDSRightMenuViewController ()

@end

@implementation iNDSRightMenuViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
        
    }
    return self;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    self.titleLabel.text = self.game.title;
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
    SASlideMenuRootViewController *rootViewController = (SASlideMenuRootViewController*)[UIApplication sharedApplication].keyWindow.rootViewController;
    [rootViewController removeRightMenu];
}

#pragma mark - Table View data source

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return 1 + self.game.numberOfSaveStates;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"DetailCell"];
    
    cell.backgroundView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"row"]];
    cell.selectedBackgroundView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"rowselected"]];
    cell.accessoryView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"disclosure"]];
    
    // text
    cell.textLabel.text = NSLocalizedString(@"LAUNCH_GAME", nil);
    cell.detailTextLabel.text = NSLocalizedString(@"LAUNCH_GAME_DETAIL", nil);
    
    // detail
    if (indexPath.row > 0) {
        cell.textLabel.text = (indexPath.row == 1 && self.game.hasPauseState) ? NSLocalizedString(@"RESUME_AUTOSAVE", nil) : [self.game nameOfSaveStateAtIndex:indexPath.row - 1];
        cell.detailTextLabel.text = [NSDateFormatter localizedStringFromDate:[self.game dateOfSaveStateAtIndex:indexPath.row -1]
                                                                   dateStyle:NSDateFormatterMediumStyle
                                                                   timeStyle:NSDateFormatterMediumStyle];
    }
    
    return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return indexPath.row > 0;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        if ([self.game deleteSaveStateAtIndex:indexPath.row - 1]) [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        if (self.game.numberOfSaveStates == 0) [(SASlideMenuRootViewController*)self.parentViewController doSlideIn:nil];
    }
}

#pragma mark - Table View delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [AppDelegate.sharedInstance startGame:self.game withSavedState:indexPath.row - 1];
}

@end

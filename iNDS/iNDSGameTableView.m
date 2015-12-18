//
//  iNDSGameTableView.m
//  iNDS
//
//  Created by Will Cobb on 11/4/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSGameTableView.h"
#import "iNDSEmulatorViewController.h"
#import "SCLAlertView.h"
@interface iNDSGameTableView() {
    
}
@end

@implementation iNDSGameTableView

-(void) viewDidLoad
{
    [super viewDidLoad];
    self.navigationItem.title = _game.gameTitle;
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.tableView reloadData];
    
}

#pragma mark - Table View

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return indexPath.section != 0;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    if (section == 0)
        return 1;
    return _game.saveStates.count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 2;
}

-(NSArray *)tableView:(UITableView *)tableView editActionsForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    UITableViewRowAction *renameAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"rename" handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        SCLAlertView * alert = [[SCLAlertView alloc] init];
        NSLog(@"Save %@", _game.saveStates[indexPath.row]);
        UITextField *textField = [alert addTextField:@"Hey"];
        
        [alert addButton:@"Show Name" actionBlock:^(void) {
            NSLog(@"Text value: %@", textField.text);
        }];
        //[alert showEdit:self title:@"Edit View" subTitle:@"This alert view shows a text box" closeButtonTitle:@"Done" duration:0.0f];
    }];
    renameAction.backgroundColor = [UIColor colorWithRed:85/255.0 green:175/255.0 blue:238/255.0 alpha:1];
    
    UITableViewRowAction *deleteAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"Delete"  handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        if ([_game deleteSaveStateAtIndex:indexPath.row]) {
            [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        } else {
            NSLog(@"Error! unable to delete save state");
        }
    }];
    deleteAction.backgroundColor = [UIColor redColor];
    return @[deleteAction, renameAction];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 0) {
         UITableViewCell* cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"Launch"];
        cell.textLabel.text = @"Launch Normally";
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        return cell;
    }
    else if (indexPath.section == 1) {
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Cell"];
        if (!cell) {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"Cell"];
        }
        // Name
        NSString * saveStateTitle = [_game nameOfSaveStateAtIndex:indexPath.row];
        if ([saveStateTitle isEqualToString:@"pause"]) {
            saveStateTitle = @"Auto Save";
        }
        cell.textLabel.text = saveStateTitle;
        
        // Date
        NSDateFormatter *timeFormatter = [[NSDateFormatter alloc]init];
        timeFormatter.dateFormat = @"h:mm a, MMMM d yyyy";
        NSString * dateString = [timeFormatter stringFromDate:[_game dateOfSaveStateAtIndex:indexPath.row]];
        cell.detailTextLabel.text = dateString;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        return cell;
    }
}

#pragma mark - Select ROMs

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [AppDelegate.sharedInstance startGame:_game withSavedState:indexPath.section == 0 ? -1 : indexPath.row];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
}


@end

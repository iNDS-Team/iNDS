//
//  iNDSGameTableView.m
//  iNDS
//
//  Created by Will Cobb on 11/4/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSGameTableView.h"
#import "iNDSEmulatorViewController.h"
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

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        if ([_game deleteSaveStateAtIndex:indexPath.row]) {
            [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        } else {
            NSLog(@"Error! unable to delete save state");
        }
    }
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

- (NSArray *)tableView:(UITableView *)tableView editActionsForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return nil;
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

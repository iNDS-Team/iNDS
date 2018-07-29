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
#import "CHBgDropboxSync.h"
@interface iNDSGameTableView() {
    
}
@end

@implementation iNDSGameTableView

- (void)configUI
{
    self.tableView.tableFooterView = [UIView new];
}

-(void) viewDidLoad
{
    [super viewDidLoad];
    [self configUI];
    self.navigationItem.title = _game.gameTitle;
    
    [[AppDelegate sharedInstance].currentEmulatorViewController.settingsContainer addObserver:self forKeyPath:@"hidden" options:NSKeyValueObservingOptionNew context:nil];
}

-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.tableView reloadData];
    
}

- (void)dealloc
{
    [[AppDelegate sharedInstance].currentEmulatorViewController.settingsContainer removeObserver:self forKeyPath:@"hidden" context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
    if (object == [AppDelegate sharedInstance].currentEmulatorViewController.settingsContainer) {
        if ([change[NSKeyValueChangeNewKey] isEqual:@(NO)]) {
            [self.tableView reloadData];
        }
    }
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
    
    UITableViewRowAction *renameAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"Rename" handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
        NSLog(@"Save %@", _game.saveStates[indexPath.row]);
        UITextField *textField = [alert addTextField:@""];
        textField.text = [_game nameOfSaveStateAtIndex:indexPath.row];
        
        [alert addButton:@"Rename" actionBlock:^(void) {
            NSString *savePath = [_game pathForSaveStateAtIndex:indexPath.row];
            NSString *rootPath = savePath.stringByDeletingPathExtension.stringByDeletingPathExtension;
            NSString *dstPath = [NSString stringWithFormat:@"%@.%@.dsv", rootPath, textField.text];
            NSLog(@"%@ - \n%@", savePath, dstPath);
            
            [[NSFileManager defaultManager] moveItemAtPath:savePath toPath:dstPath error:nil];
            [CHBgDropboxSync forceStopIfRunning]; //If we delete while syncing this shitty thing crashes
            [_game reloadSaveStates];
            [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
        }];
        [alert showEdit:self title:@"Rename" subTitle:@"Rename Save State" closeButtonTitle:@"Cancel" duration:0.0f];
    }];
    renameAction.backgroundColor = [UIColor colorWithRed:1.00 green:0.76 blue:0.03 alpha:1.0];
    
    UITableViewRowAction *deleteAction = [UITableViewRowAction rowActionWithStyle:UITableViewRowActionStyleNormal title:@"Delete"  handler:^(UITableViewRowAction *action, NSIndexPath *indexPath){
        [CHBgDropboxSync forceStopIfRunning];
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
    UITableViewCell* cell;
    if (indexPath.section == 0) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"Launch"];
        cell.textLabel.text = @"Launch Normally";
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    else if (indexPath.section == 1) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"Cell"];
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
    }
    return cell;
}

#pragma mark - Select ROMs

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [AppDelegate.sharedInstance startGame:_game withSavedState:indexPath.section == 0 ? -1 : indexPath.row];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
}


@end

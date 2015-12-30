//
//  iNDSCheatsFolderTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/30/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSCheatsFolderTableViewController.h"
#include "emu.h"
#include "cheatSystem.h"
@interface iNDSCheatsFolderTableViewController ()

@end

@implementation iNDSCheatsFolderTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];

}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    cheats->save();
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [self.folderCheats count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell * cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"Cheat"];
    u32 index = (u32)[self.folderCheats[indexPath.row] integerValue];
    CHEATS_LIST *cheat = cheats->getItemByIndex(index);
    cell.accessoryType = cheat->enabled ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    NSString *description = [NSString stringWithCString:cheat->description encoding:NSUTF8StringEncoding];
    NSString *name = [description componentsSeparatedByString:@"||"][1];
    cell.textLabel.text = name;
    cell.textLabel.lineBreakMode = NSLineBreakByWordWrapping;
    cell.textLabel.numberOfLines = 0;
    cell.textLabel.font = [UIFont systemFontOfSize:14];
    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    u32 index = (u32)[self.folderCheats[indexPath.row] integerValue];
    CHEATS_LIST *cheat = cheats->getItemByIndex(index);
    NSString *description = [NSString stringWithCString:cheat->description encoding:NSUTF8StringEncoding];
    NSString *name = [description componentsSeparatedByString:@"||"][1];
    NSAttributedString *attributedText =
    [[NSAttributedString alloc] initWithString:name attributes:@{NSFontAttributeName: [UIFont systemFontOfSize:14]}];
    CGRect rect = [attributedText boundingRectWithSize:CGSizeMake(tableView.bounds.size.width, CGFLOAT_MAX)
                                               options:NSStringDrawingUsesLineFragmentOrigin
                                               context:nil];
    return rect.size.height + 20;
    
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    u32 index = (u32)[self.folderCheats[indexPath.row] integerValue];
    CHEATS_LIST *cheat = cheats->getItemByIndex(index);
    cheat->enabled = !cheat->enabled;
    [tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
    
}


@end

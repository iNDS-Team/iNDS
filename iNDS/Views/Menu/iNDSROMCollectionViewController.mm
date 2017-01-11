//
//  ViewController.m
//  iNDS
//
//  Created by Will Cobb on 1/6/17.
//  Copyright © 2017 Will Cobb. All rights reserved.
//

#import "iNDSROMCollectionViewController.h"
#import "iNDSROMCollectionController.h"
#import "iNDSROM.h"
#import "iNDSEmulationViewController.h"
#import "iNDSEmulationController.h"

@interface iNDSROMCollectionViewController ()

@property iNDSROMCollectionController *romCollection;

@end

@implementation iNDSROMCollectionViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:NO];
    self.romCollection = [[iNDSROMCollectionController alloc] init];
    [self.romCollection updateRoms];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.romCollection.roms.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"iNDSROM"];
    
    iNDSROM *rom = self.romCollection.roms[indexPath.row];
    cell.textLabel.text = rom.title;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self launchROM:self.romCollection.roms[indexPath.row]];
}

- (void)launchROM:(iNDSROM *)rom {
    NSLog(@"Launching ROM");
    iNDSEmulationController *emulationController = [[iNDSEmulationController alloc] initWithRom:rom];
    iNDSEmulationViewController *emulationViewController = [[iNDSEmulationViewController alloc] initWithEmulationController:emulationController];
    NSLog(@"%@ %@", emulationController, emulationViewController);
    [self presentViewController:emulationViewController animated:YES completion:nil];
}

@end

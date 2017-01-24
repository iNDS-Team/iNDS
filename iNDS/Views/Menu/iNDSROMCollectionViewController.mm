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
#import "iNDSROMViewCell.h"

@interface iNDSROMCollectionViewController ()

@property iNDSROMCollectionController *romCollection;

@end

@implementation iNDSROMCollectionViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:NO];
    self.romCollection = [[iNDSROMCollectionController alloc] init];
    [self.romCollection updateRoms];
    [self.collectionView registerClass:[iNDSROMViewCell class] forCellWithReuseIdentifier:@"iNDSROM"];
    
    self.collectionView.backgroundColor = [UIColor colorWithWhite:50/255.0 alpha:1];
    //self.navigationController.navigationBar.barStyle = UIBarStyleDefault;
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
}


- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return self.romCollection.roms.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    iNDSROMViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"iNDSROM" forIndexPath:indexPath];
    
    iNDSROM *rom = self.romCollection.roms[indexPath.row];
    
    cell.rom = rom;
    
    return cell;
}

- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
    return CGSizeMake(90, 150);
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    [collectionView deselectItemAtIndexPath:indexPath animated:YES];
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

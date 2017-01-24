//
//  iNDSEmulationMenuView.m
//  iNDS
//
//  Created by Will Cobb on 1/19/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationMenuView.h"
@interface iNDSEmulationMenuView () <UICollectionViewDelegate, UICollectionViewDataSource>

@end

@implementation iNDSEmulationMenuView

- (id)initWithFrame:(CGRect)frame  {
    UICollectionViewFlowLayout* flowLayout = [[UICollectionViewFlowLayout alloc] init];
    flowLayout.itemSize = CGSizeMake(80, 80);
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionHorizontal];
    
    if (self = [super initWithFrame:frame collectionViewLayout:flowLayout]) {
        [self registerClass:[UICollectionViewCell class] forCellWithReuseIdentifier:@"Cell"];
        self.backgroundColor = [UIColor clearColor];
        
        self.dataSource = self;
        self.delegate = self;
    }
    return self;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return 4;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    UICollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:@"Cell" forIndexPath:indexPath];
    
    cell.backgroundColor = [UIColor clearColor];
    cell.layer.cornerRadius = 5;
    cell.layer.borderColor = [UIColor whiteColor].CGColor;
    cell.layer.borderWidth = 3.0f;
    cell.alpha = 0.7;
    
    NSLog(@"%@", cell);
    
    return cell;
}

//- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
//    return CGSizeMake(50, 50);
//}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    [collectionView deselectItemAtIndexPath:indexPath animated:YES];
    
}

@end

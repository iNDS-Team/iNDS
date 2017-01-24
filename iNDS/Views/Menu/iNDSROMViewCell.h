//
//  CollectionViewCell.h
//  iNDS
//
//  Created by Will Cobb on 1/14/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <UIKit/UIKit.h>

@class iNDSROM;
@interface iNDSROMViewCell : UICollectionViewCell

@property (nonatomic, weak) iNDSROM *rom;

@end

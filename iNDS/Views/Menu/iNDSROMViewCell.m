//
//  CollectionViewCell.m
//  iNDS
//
//  Created by Will Cobb on 1/14/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSROMViewCell.h"
#import "iNDSROM.h"

@interface iNDSROMViewCell ()

@property UIImageView   *artworkImageView;
@property UILabel       *titleLabel;

@end

@implementation iNDSROMViewCell

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.artworkImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, frame.size.width, frame.size.width)];
        [self.contentView addSubview:self.artworkImageView];
        
        self.titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, frame.size.width, frame.size.width, frame.size.height - frame.size.width)];
        self.titleLabel.font = [UIFont systemFontOfSize:10 weight:0.1];
        self.titleLabel.numberOfLines = 0;
        self.titleLabel.textAlignment = NSTextAlignmentCenter;
        self.titleLabel.textColor = [UIColor colorWithWhite:1 alpha:0.6];
        [self.contentView addSubview:self.titleLabel];
    }
    return self;
}

- (void)setRom:(iNDSROM *)rom {
    _rom = rom;
    self.artworkImageView.image = rom.icon;
    
    self.titleLabel.frame = CGRectMake(0, self.frame.size.width, self.frame.size.width, self.frame.size.height - self.frame.size.width);
    self.titleLabel.text = rom.gameTitle;
    [self.titleLabel sizeToFit];
}

@end

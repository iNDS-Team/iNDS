//
//  WCEasySettingsButton.m
//  iNDS
//
//  Created by Frederick Morlock on 7/11/18.
//  Copyright © 2018 iNDS. All rights reserved.
//

#import "WCEasySettingsButton.h"

@implementation WCEasySettingsButton

- (id)initWithTitle:(NSString *)title subtitle:(NSString *)subtitle  callback:(inputCallback)completion
{
    if (self = [super initWithIdentifier:nil title:title]) {
        self.callback = completion;
        self.subtitle = subtitle;
        self.type = WCEasySettingsTypeUrl;
        self.cellIdentifier = @"Reset";
    }
    return self;
}


- (void)itemSelected
{
    if (self.callback) {
        self.callback(TRUE);
    }
}

@end

@implementation WCEasySettingsButtonCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    if (self = [super initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:reuseIdentifier]) {
        
    }
    return self;
}

- (void)setController:(WCEasySettingsButton *)controller
{
    self.textLabel.text = controller.title;
    self.textLabel.textColor = self.tintColor;
    self.detailTextLabel.text = controller.subtitle;
    self.accessoryType = UITableViewCellAccessoryNone;
    self.selectionStyle = UITableViewCellSelectionStyleDefault;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];
}

@end

//
//  WCEasySettingsSlider.h
//  iNDS
//
//  Created by Will Cobb on 4/20/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import "WCEasySettingsItem.h"

@interface WCEasySettingsSlider2 : WCEasySettingsItem

- (id)initWithIdentifier:(NSString *)identifier title:(NSString *)title max:(int)max;@property NSString          *cellIdentifier;
- (void)onSlide:(UISlider *)s;
@property int max;

@end

@interface WCEasySettingsSlider2Cell : WCEasySettingsItemCell {
    UILabel     *cellTitle;
    UISlider    *cellSlider;
    UILabel     *sliderPercentage;
}


@end

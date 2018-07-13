//
//  WCEasySettingsButton.h
//  iNDS
//
//  Created by Frederick Morlock on 7/11/18.
//  Copyright © 2018 iNDS. All rights reserved.
//

#import "WCEasySettingsItem.h"

typedef void(^inputCallback)(bool finished);

@interface WCEasySettingsButton : WCEasySettingsItem

@property (nonatomic, copy) inputCallback callback;
@property NSString *subtitle;

- (id)initWithTitle:(NSString *)title subtitle:(NSString *)subtitle  callback:(inputCallback)completion;

@end

@interface WCEasySettingsButtonCell : WCEasySettingsItemCell {
}

@end


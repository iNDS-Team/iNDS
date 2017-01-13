//
//  iNDSGamePadView.h
//  iNDS
//
//  Created by Will Cobb on 1/10/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <UIKit/UIKit.h>

@class iNDSEmulationProfile;

@interface iNDSGamePadView : UIView

@property (nonatomic, strong) iNDSEmulationProfile *profile;

@end

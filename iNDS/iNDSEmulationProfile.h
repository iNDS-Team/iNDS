//
//  iNDSEmulationProfile.h
//  iNDS
//
//  Created by Will Cobb on 12/2/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface iNDSEmulationProfile : NSObject

@property (strong, nonatomic, readonly) NSString* name;
@property (assign, nonatomic, readonly) UIUserInterfaceIdiom device;
@property (assign, nonatomic) CGSize screenSize;
@property (assign, nonatomic) CGRect mainScreenRect;
@property (assign, nonatomic) CGRect touchScreenRect;
@property (assign, nonatomic) CGRect leftTrigger;
@property (assign, nonatomic) CGRect rightTrigger;
@property (assign, nonatomic) CGRect directionalControl;
@property (assign, nonatomic) CGRect buttonControl;
@property (assign, nonatomic) CGRect startButton;
@property (assign, nonatomic) CGRect selectButton;

- (id)init;
- (CGRect)mainScreenRect;
- (CGRect)touchScreenRect;
- (CGRect)leftTrigger;
- (CGRect)rightTrigger;
- (CGRect)directionalControl;
- (CGRect)buttonControl;
- (CGRect)startButton;
- (CGRect)selectButton;
- (CGRect)settingsButton;
@end

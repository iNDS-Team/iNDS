//
//  iNDSEmulationProfile.h
//  iNDS
//
//  Created by Will Cobb on 12/2/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "iNDSButtonControl.h"
#import "iNDSDirectionalControl.h"

@interface iNDSEmulationProfile : NSObject

@property (strong, nonatomic, readonly) NSString* name;
@property (assign, nonatomic, readonly) UIUserInterfaceIdiom device;

@property (assign, nonatomic) CGSize screenSize;

@property (weak, nonatomic) UIView *mainScreen;
@property (weak, nonatomic) UIView *touchScreen;
@property (weak, nonatomic) UILabel *fpsLabel;
@property (weak, nonatomic) UIButton *settingsButton;
@property (weak, nonatomic) UIButton *startButton;
@property (weak, nonatomic) UIButton *selectButton;
@property (weak, nonatomic) UIButton *leftTrigger;
@property (weak, nonatomic) UIButton *rightTrigger;
@property (weak, nonatomic) iNDSDirectionalControl *directionalControl;
@property (weak, nonatomic) iNDSButtonControl *buttonControl;


- (id)initWithProfileName:(NSString*) name;
- (void)ajustLayout;
@end


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

@property (strong, nonatomic) NSString* name;

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

@property (weak, nonatomic) UISlider *sizeSlider;


- (id)initWithProfileName:(NSString*) name;
+ (NSArray*)profilesAtPath:(NSString*)profilesPath;
+ (iNDSEmulationProfile *)profileWithPath:(NSString*)path;
- (void)ajustLayout;
- (void)enterEditMode;
- (void)handlePan:(UIView *)currentView Location:(CGPoint) location state:(UIGestureRecognizerState) state;
- (void)sizeChanged:(UISlider *)sender;
- (void)exitEditMode;
- (void)saveProfile;
- (BOOL)deleteProfile;
+ (NSString*)pathForProfileName:(NSString *)name;
@end


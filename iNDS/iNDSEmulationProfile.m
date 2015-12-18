//
//  iNDSEmulationProfile.m
//  iNDS
//
//  Created by Will Cobb on 12/2/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSEmulationProfile.h"


@interface iNDSEmulationProfile()
{
    CGRect mainScreenRects[2];
    CGRect touchScreenRects[2];
    CGRect leftTriggers[2];
    CGRect rightTriggers[2];
    CGRect directionalControls[2];
    CGRect buttonControls[2];
    CGRect startButtons[2];
    CGRect selectButtons[2];
    CGRect settingsButtons[2];
}
@end

@implementation iNDSEmulationProfile

- (id)initWithProfileName:(NSString*) name
{
    if (self = [super init]) {
        for (int i = 0; i < 8; i++) { //initialize frames
            settingsButtons[i] = CGRectMake(5, 5, 22, 22);
            startButtons[i] = selectButtons[i] = CGRectMake(0, 0, 48, 28);
            leftTriggers[i] = rightTriggers[i] = CGRectMake(0, 0, 67, 44);
            directionalControls[i] = buttonControls[i] = CGRectMake(0, 0, 120, 120);
        }
        // Setup the default screen profile
        _name = name;
        
        CGSize gameScreenSize = CGSizeMake(self.screenSize.width, self.screenSize.width * 0.75);
        
        NSInteger view = 0; //0 Portait, 1 Landscape
        // Portrait
        startButtons[view].origin = CGPointMake(self.screenSize.width/2 - 48 - 7, self.screenSize.height - 28 - 7);
        selectButtons[view].origin = CGPointMake(self.screenSize.width/2 + 7, self.screenSize.height - 28 - 7);
        mainScreenRects[view] = CGRectMake(0, (self.screenSize.height / 2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
        touchScreenRects[view] = CGRectMake(0, self.screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
        leftTriggers[view].origin = CGPointMake(0, self.screenSize.height/2);
        rightTriggers[view].origin = CGPointMake(self.screenSize.width - 67    , self.screenSize.height/2);
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            directionalControls[view].origin = CGPointMake(10, self.screenSize.height - 130);
            buttonControls[view].origin = CGPointMake(self.screenSize.width - 120 - 10, self.screenSize.height - 130);
        } else {
            directionalControls[view].origin = CGPointMake(10, self.screenSize.height - 160);
            buttonControls[view].origin = CGPointMake(self.screenSize.width - 120 - 10, self.screenSize.height - 160);
        }
        // Landscape
        gameScreenSize = CGSizeMake(self.screenSize.height, self.screenSize.height * 0.75);
        view = 1;
        mainScreenRects[view] = CGRectMake(0, (self.screenSize.height/2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
        touchScreenRects[view] = CGRectMake(0, self.screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            startButtons[view].origin = CGPointMake(self.screenSize.width * (3/4.0), self.screenSize.height - 28 - 47);
            selectButtons[view].origin = CGPointMake(self.screenSize.width * (3/4.0), self.screenSize.height - 28 - 7);
            directionalControls[view].origin = CGPointMake(10, self.screenSize.height/2 - 60);
            buttonControls[view].origin = CGPointMake(self.screenSize.width - 120 - 10, self.screenSize.height/2 - 60);
        } else {
            startButtons[view].origin = CGPointMake(self.screenSize.width - 60, self.screenSize.height/2 + 80);
            selectButtons[view].origin = CGPointMake(self.screenSize.width - 60, self.screenSize.height/2 + 115);
            directionalControls[view].origin = CGPointMake(20, self.screenSize.height/2 - 60);
            buttonControls[view].origin = CGPointMake(self.screenSize.width - 120 - 20, self.screenSize.height/2 - 60);
        }
    }
    return self;
}
        
-(CGSize)currentScreenSize
{ 
    CGRect screenBounds = [UIScreen mainScreen].bounds ;
    CGFloat width = CGRectGetWidth(screenBounds)  ;
    CGFloat height = CGRectGetHeight(screenBounds) ;

    if ([self isPortrait]){
        return CGSizeMake(width, height);
    }
    return CGSizeMake(height, width);
}

-(BOOL) isPortrait
{
    return UIInterfaceOrientationIsPortrait([UIApplication sharedApplication].statusBarOrientation);
}


- (CGRect) mainScreenRect
{
    return mainScreenRects[![self isPortrait]];
}
- (CGRect)touchScreenRect
{
    return touchScreenRects[![self isPortrait]];
}
- (CGRect)leftTrigger
{
    return leftTriggers[![self isPortrait]];
}
- (CGRect)rightTrigger
{
    return rightTriggers[![self isPortrait]];
}
- (CGRect)directionalControl
{
    return directionalControls[![self isPortrait]];
}
- (CGRect)buttonControl
{
    return buttonControls[![self isPortrait]];
}
- (CGRect)startButton
{
    return startButtons[![self isPortrait]];
}
- (CGRect)selectButton
{
    return selectButtons[![self isPortrait]];
}
- (CGRect)settingsButton
{
    return settingsButtons[![self isPortrait]];
}


@end

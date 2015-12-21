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
    CGRect leftTriggerRects[2];
    CGRect rightTriggerRects[2];
    CGRect directionalControlRects[2];
    CGRect buttonControlRects[2];
    CGRect startButtonRects[2];
    CGRect selectButtonRects[2];
    CGRect settingsButtonRects[2];
    CGRect fpsLabelRects[2];
}
@end

@implementation iNDSEmulationProfile

- (id)initWithProfileName:(NSString*) name
{
    if (self = [super init]) {
        for (int i = 0; i < 2; i++) { //initialize frames
            settingsButtonRects[i] = CGRectMake(0, 0, 40, 40);
            startButtonRects[i] = selectButtonRects[i] = CGRectMake(0, 0, 48, 28);
            leftTriggerRects[i] = rightTriggerRects[i] = CGRectMake(0, 0, 67, 44);
            directionalControlRects[i] = buttonControlRects[i] = CGRectMake(0, 0, 120, 120);
            fpsLabelRects[i] = CGRectMake(40, 5, 70, 24);
        }
        // Setup the default screen profile
        _name = name;
        
        CGSize screenSize = [self currentScreenSize];
        CGSize gameScreenSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333 * 0.5), MIN(screenSize.width, screenSize.height * 1.333 * 0.5) * 0.75); //Bound the screens by height or width
        NSInteger view = 0; //0 Portait, 1 Landscape
        // Portrait
        mainScreenRects[view] = CGRectMake(0, (screenSize.height / 2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
        touchScreenRects[view] = CGRectMake(0, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
        startButtonRects[view].origin = CGPointMake(screenSize.width/2 - 48 - 7, screenSize.height - 28 - 7);
        selectButtonRects[view].origin = CGPointMake(screenSize.width/2 + 7, screenSize.height - 28 - 7);
        leftTriggerRects[view].origin = CGPointMake(0, screenSize.height/2);
        rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67, screenSize.height/2);
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            directionalControlRects[view].origin = CGPointMake(10, screenSize.height - 130);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height - 130);
        } else {
            directionalControlRects[view].origin = CGPointMake(10, screenSize.height - 160);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height - 160);
        }
        
        // Landscape
        screenSize = CGSizeMake(screenSize.height, screenSize.width);
        gameScreenSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333 * 0.5), MIN(screenSize.width, screenSize.height * 1.333 * 0.5) * 0.75);
        
        view = 1;
        
        mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height/2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
        touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
        leftTriggerRects[view].origin = CGPointMake(0, screenSize.height/4);
        rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67, screenSize.height/4);
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            startButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0), screenSize.height - 28 - 47);
            selectButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0), screenSize.height - 28 - 7);
            directionalControlRects[view].origin = CGPointMake(10, screenSize.height/2);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height/2);
        } else {
            startButtonRects[view].origin = CGPointMake(screenSize.width - 60, screenSize.height/2 + 80);
            selectButtonRects[view].origin = CGPointMake(screenSize.width - 60, screenSize.height/2 + 115);
            directionalControlRects[view].origin = CGPointMake(20, screenSize.height/2 - 60);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 20, screenSize.height/2 - 60);
        }
    }
    return self;
}

- (void)ajustLayout
{
    self.mainScreen.frame = mainScreenRects[![self isPortrait]];
    self.touchScreen.frame = touchScreenRects[![self isPortrait]];
    self.startButton.frame = startButtonRects[![self isPortrait]];
    self.selectButton.frame = selectButtonRects[![self isPortrait]];
    self.leftTrigger.frame = leftTriggerRects[![self isPortrait]];
    self.rightTrigger.frame = rightTriggerRects[![self isPortrait]];
    self.directionalControl.frame = directionalControlRects[![self isPortrait]];
    self.buttonControl.frame = buttonControlRects[![self isPortrait]];
    self.settingsButton.frame = settingsButtonRects[![self isPortrait]];
    self.fpsLabel.frame = fpsLabelRects[![self isPortrait]];
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




@end

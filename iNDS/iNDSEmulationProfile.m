//
//  iNDSEmulationProfile.m
//  iNDS
//
//  Created by Will Cobb on 12/2/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSEmulationProfile.h"
#import "AppDelegate.h"
#import "SCLAlertView.h"
#import "NSString+CompareToVersion.h"
#import "CHBgDropboxSync.h"

#define HORIZONTAL 1
#define VERTICAL 2

#define VERSION @"1.0.2"

static CGFloat const iPhoneXBottomSafeAreaWhenLandscape = 21.f;
static CGFloat const iPhoneXHorizontalMarginWhenLandscape = 44.f;
static CGFloat const OffsetYForScreenOniPhoneX = 80.f;
static CGFloat const OffsetYForStartButtonOniPhoneX = 50.f;
static CGFloat const OffsetYForSelectButtonOniPhoneX = 50.f;
static CGFloat const OffsetYForDirectionControlOniPhoneX = 65.f;
static CGFloat const OffsetYForButtonControlOniPhoneX = 65.f;
static CGFloat const OffsetYForTriggerOniPhoneX = 130.f;

@interface iNDSEmulationProfile()
{
    NSString *profileVersion;
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
    
    CGFloat offsetX, offsetY;
    iNDSEmulatorViewController * emulationView;
    
    UIView * selectedView;
    
}
@end

@implementation iNDSEmulationProfile

- (id)initWithProfileName:(NSString*) name
{
    if (self = [super init]) {
        self.name = name;
        self.screenSize = [self currentScreenSizeAlwaysPortrait:YES];
        profileVersion = VERSION;
        for (int i = 0; i < 2; i++) { //initialize frames
            settingsButtonRects[i] = CGRectMake(0, 0, 40, 40);
            startButtonRects[i] = selectButtonRects[i] = CGRectMake(0, 0, 48, 28);
            leftTriggerRects[i] = rightTriggerRects[i] = CGRectMake(0, 0, 67, 44);
            directionalControlRects[i] = buttonControlRects[i] = CGRectMake(0, 0, 120, 120);
            fpsLabelRects[i] = CGRectMake(40, 5, 240, 24);
        }
        // Setup the default screen profile
        _name = name;
        
        CGSize screenSize = [self currentScreenSizeAlwaysPortrait:YES];
        CGSize gameScreenSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333 * 0.5), MIN(screenSize.width, screenSize.height * 1.333 * 0.5) * 0.75); //Bound the screens by height or width
        NSInteger view = 0; //0 Portait, 1 Landscape
        // Portrait
        if (IsiPhoneX) {
            fpsLabelRects[view] = CGRectMake(10, 5, 240, 48);
            settingsButtonRects[view] = CGRectMake(screenSize.width - 10 - 40, 5, 40, 40);
            mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height / 2) - gameScreenSize.height - OffsetYForScreenOniPhoneX, gameScreenSize.width, gameScreenSize.height);
            touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2 - OffsetYForScreenOniPhoneX, gameScreenSize.width, gameScreenSize.height);
            
            
            startButtonRects[view].origin = CGPointMake(screenSize.width/2 - 48 - 7, screenSize.height - 28 - 7 - OffsetYForStartButtonOniPhoneX);
            selectButtonRects[view].origin = CGPointMake(screenSize.width/2 + 7, screenSize.height - 28 - 7 - OffsetYForSelectButtonOniPhoneX);
            leftTriggerRects[view].origin = CGPointMake(0, screenSize.height/2 - OffsetYForTriggerOniPhoneX);
            rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67, screenSize.height/2 - OffsetYForTriggerOniPhoneX);
        } else {
            mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height / 2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
            touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
            startButtonRects[view].origin = CGPointMake(screenSize.width/2 - 48 - 7, screenSize.height - 28 - 7);
            selectButtonRects[view].origin = CGPointMake(screenSize.width/2 + 7, screenSize.height - 28 - 7);
            leftTriggerRects[view].origin = CGPointMake(0, screenSize.height/2);
            rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67, screenSize.height/2);
        }
        
        
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            if (IsiPhoneX) {
                directionalControlRects[view].origin = CGPointMake(10, screenSize.height - 130 - OffsetYForDirectionControlOniPhoneX);
                buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height - 130 - OffsetYForButtonControlOniPhoneX);
            } else {
                directionalControlRects[view].origin = CGPointMake(10, screenSize.height - 130);
                buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height - 130);
            }
        } else {
            directionalControlRects[view].origin = CGPointMake(10, screenSize.height - 160);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height - 160);
        }
        
        // Landscape
        screenSize = CGSizeMake(screenSize.height, screenSize.width);
        if (IsiPhoneX) {
            gameScreenSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333 * 0.5), MIN(screenSize.width, screenSize.height * 1.333 * 0.5) * 0.75);
            gameScreenSize = CGSizeMake(gameScreenSize.width-iPhoneXBottomSafeAreaWhenLandscape, gameScreenSize.height-iPhoneXBottomSafeAreaWhenLandscape);
        } else {
            gameScreenSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333 * 0.5), MIN(screenSize.width, screenSize.height * 1.333 * 0.5) * 0.75);
        }
        
        view = 1;
        
        leftTriggerRects[view].origin = CGPointMake(0, screenSize.height/4);
        rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67, screenSize.height/4);
        
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            if (IsiPhoneX) {
                fpsLabelRects[view] = CGRectMake(iPhoneXHorizontalMarginWhenLandscape, 5, 240, 48);
                settingsButtonRects[view] = CGRectMake(screenSize.width - iPhoneXHorizontalMarginWhenLandscape - 40, 5, 40, 40);
                leftTriggerRects[view].origin = CGPointMake(iPhoneXHorizontalMarginWhenLandscape, screenSize.height/4);
                rightTriggerRects[view].origin = CGPointMake(screenSize.width - 67 - iPhoneXHorizontalMarginWhenLandscape, screenSize.height/4);
                startButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0) - iPhoneXHorizontalMarginWhenLandscape, screenSize.height - 28 - 47 - iPhoneXBottomSafeAreaWhenLandscape);
                selectButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0) - iPhoneXHorizontalMarginWhenLandscape, screenSize.height - 28 - 7 - iPhoneXBottomSafeAreaWhenLandscape);
                directionalControlRects[view].origin = CGPointMake(iPhoneXHorizontalMarginWhenLandscape, screenSize.height/2);
                buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - iPhoneXHorizontalMarginWhenLandscape, screenSize.height/2);
                mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height/2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
                touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
            } else {
                startButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0), screenSize.height - 28 - 47);
                selectButtonRects[view].origin = CGPointMake(screenSize.width * (3/4.0), screenSize.height - 28 - 7);
                directionalControlRects[view].origin = CGPointMake(10, screenSize.height/2);
                buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 10, screenSize.height/2);
                mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height/2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
                touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
            }
        } else {
            startButtonRects[view].origin = CGPointMake(screenSize.width - 60, screenSize.height/2 + 80);
            selectButtonRects[view].origin = CGPointMake(screenSize.width - 60, screenSize.height/2 + 115);
            directionalControlRects[view].origin = CGPointMake(20, screenSize.height/2 - 60);
            buttonControlRects[view].origin = CGPointMake(screenSize.width - 120 - 20, screenSize.height/2 - 60);
            mainScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, (screenSize.height/2) - gameScreenSize.height, gameScreenSize.width, gameScreenSize.height);
            touchScreenRects[view] = CGRectMake(screenSize.width/2 - gameScreenSize.width/2, screenSize.height/2, gameScreenSize.width, gameScreenSize.height);
        }
        
        emulationView = [AppDelegate sharedInstance].currentEmulatorViewController;
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)coder {
    self = [self initWithProfileName:[coder decodeObjectForKey:@"iNDSProfileName"]];
    if (self) {
        NSString * thisVersion = [coder decodeObjectForKey:@"iNDSProfileVersion"];
        if ([VERSION isEqualOrNewerThanVersion:thisVersion]) { // Was this profile
            //Probably a better way to do this but I wanted to have support for later versions
            mainScreenRects[0] = [coder decodeCGRectForKey:@"iNDSProfileMainScreenRectPortrait"];
            mainScreenRects[1] = [coder decodeCGRectForKey:@"iNDSProfileMainScreenRectLandscape"];
            touchScreenRects[0] = [coder decodeCGRectForKey:@"iNDSProfileTouchScreenRectPortrait"];
            touchScreenRects[1] = [coder decodeCGRectForKey:@"iNDSProfileTouchScreenRectLandscape"];
            leftTriggerRects[0] = [coder decodeCGRectForKey:@"iNDSLeftTriggerRectPortrait"];
            leftTriggerRects[1] = [coder decodeCGRectForKey:@"iNDSLeftTriggerRectLandscape"];
            rightTriggerRects[0] = [coder decodeCGRectForKey:@"iNDSRightTriggerRectPortrait"];
            rightTriggerRects[1] = [coder decodeCGRectForKey:@"iNDSRightTriggerRectLandscape"];
            buttonControlRects[0] = [coder decodeCGRectForKey:@"iNDSButtonControlRectPortrait"];
            buttonControlRects[1] = [coder decodeCGRectForKey:@"iNDSButtonControlRectLandscape"];
            directionalControlRects[0] = [coder decodeCGRectForKey:@"iNDSDirectionalControlRectPortrait"];
            directionalControlRects[1] = [coder decodeCGRectForKey:@"iNDSDirectionalControlRectLandscape"];
            startButtonRects[0] = [coder decodeCGRectForKey:@"iNDSStartButtonRectPortrait"];
            startButtonRects[1] = [coder decodeCGRectForKey:@"iNDSStartButtonRectLandscape"];
            selectButtonRects[0] = [coder decodeCGRectForKey:@"iNDSSelectButtonRectPortrait"];
            selectButtonRects[1] = [coder decodeCGRectForKey:@"iNDSSelectButtonRectLandscape"];
            settingsButtonRects[0] = [coder decodeCGRectForKey:@"iNDSSettingsButtonRectPortrait"];
            settingsButtonRects[1] = [coder decodeCGRectForKey:@"iNDSSettingsButtonRectLandscape"];
            fpsLabelRects[0] = [coder decodeCGRectForKey:@"iNDSfpsLabelRectPortrait"];
            fpsLabelRects[1] = [coder decodeCGRectForKey:@"iNDSfpsLabelRectLandscape"];
            
            //after everyone has updated we can remove the conditionals and just make then default
            if ([thisVersion isEqualOrNewerThanVersion:@"1.0.2"]) { //iNDS 1.5.0
                for (int i = 0; i < 2; i++) {
                    fpsLabelRects[i] = CGRectMake(40, 5, 240, 24);
                }
            }
            
            if ([VERSION isNewerThanVersion:thisVersion]) { //The loaded version is less, we should resave it and get it up to date
                NSLog(@"Updating Profile %@ from %@ to %@", self.name, thisVersion, VERSION);
                NSString * savePath = [iNDSEmulationProfile pathForProfileName:self.name];
                [NSKeyedArchiver archiveRootObject:self toFile:savePath];
            }
            
        } else {
            NSLog(@"Oh no, %@ was created with a newer version of iNDS", self.name);
            //Use this to alert every once and awhile that out of date profiles were not loaded.
            CFTimeInterval lastVersionAlert = [[NSUserDefaults standardUserDefaults] integerForKey:@"iNDSLastOutOfDateProfileAlert"];
            if (CACurrentMediaTime() - lastVersionAlert > 500) {
                SCLAlertView * alert = [[SCLAlertView alloc] init];
                [alert showInfo:emulationView title:@"Unable to load Profiles" subTitle:@"Some profiles were used with a later version of iNDS. These profiles will be hidded until you upgrade iNDS." closeButtonTitle:@"Thanks" duration:0.0];
                [[NSUserDefaults standardUserDefaults] setInteger:CACurrentMediaTime() forKey:@"iNDSLastOutOfDateProfileAlert"];
            }
            return nil;
        }
    }
    return self;
}

-(void)encodeWithCoder:(NSCoder *)coder{
    [coder encodeObject:self.name forKey:@"iNDSProfileName"];
    [coder encodeObject:VERSION forKey:@"iNDSProfileVersion"];
    [coder encodeCGRect:mainScreenRects[0] forKey:@"iNDSProfileMainScreenRectPortrait"];
    [coder encodeCGRect:mainScreenRects[1] forKey:@"iNDSProfileMainScreenRectLandscape"];
    [coder encodeCGRect:touchScreenRects[0] forKey:@"iNDSProfileTouchScreenRectPortrait"];
    [coder encodeCGRect:touchScreenRects[1] forKey:@"iNDSProfileTouchScreenRectLandscape"];
    [coder encodeCGRect:leftTriggerRects[0] forKey:@"iNDSLeftTriggerRectPortrait"];
    [coder encodeCGRect:leftTriggerRects[1] forKey:@"iNDSLeftTriggerRectLandscape"];
    [coder encodeCGRect:rightTriggerRects[0] forKey:@"iNDSRightTriggerRectPortrait"];
    [coder encodeCGRect:rightTriggerRects[1] forKey:@"iNDSRightTriggerRectLandscape"];
    [coder encodeCGRect:buttonControlRects[0] forKey:@"iNDSButtonControlRectPortrait"];
    [coder encodeCGRect:buttonControlRects[1] forKey:@"iNDSButtonControlRectLandscape"];
    [coder encodeCGRect:directionalControlRects[0] forKey:@"iNDSDirectionalControlRectPortrait"];
    [coder encodeCGRect:directionalControlRects[1] forKey:@"iNDSDirectionalControlRectLandscape"];
    [coder encodeCGRect:startButtonRects[0] forKey:@"iNDSStartButtonRectPortrait"];
    [coder encodeCGRect:startButtonRects[1] forKey:@"iNDSStartButtonRectLandscape"];
    [coder encodeCGRect:selectButtonRects[0] forKey:@"iNDSSelectButtonRectPortrait"];
    [coder encodeCGRect:selectButtonRects[1] forKey:@"iNDSSelectButtonRectLandscape"];
    [coder encodeCGRect:settingsButtonRects[0] forKey:@"iNDSSettingsButtonRectPortrait"];
    [coder encodeCGRect:settingsButtonRects[1] forKey:@"iNDSSettingsButtonRectLandscape"];
    [coder encodeCGRect:fpsLabelRects[0] forKey:@"iNDSfpsLabelRectPortrait"];
    [coder encodeCGRect:fpsLabelRects[1] forKey:@"iNDSfpsLabelRectLandscape"];
    [coder encodeCGSize:self.screenSize forKey:@"iNDSScreenSize"];
}

+ (NSArray*)profilesAtPath:(NSString*)profilesPath
{
    NSMutableArray *profiles = [NSMutableArray new];
    NSArray *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:profilesPath error:NULL];
    for(NSString *file in files) {
        if ([file.pathExtension isEqualToString:@"ips"]) {
            iNDSEmulationProfile *profile = [self profileWithPath:[profilesPath stringByAppendingPathComponent:file]];
            if (profile) [profiles addObject:profile];
        }
    }
    
    return [NSArray arrayWithArray:profiles];
}

+ (iNDSEmulationProfile *)profileWithPath:(NSString*)path
{
    return [NSKeyedUnarchiver unarchiveObjectWithFile:path];
}

- (void)saveProfileWithCancel:(BOOL)showCancel
{
    
    SCLAlertView *alert = [[SCLAlertView alloc] init];
    
    UITextField *textField = [alert addTextField:@""];
    textField.text = [self.name isEqualToString:@"iNDSDefaultProfile"] ? @"" : self.name;
    
    [alert addButton:@"Save" actionBlock:^(void) {
        self.name = textField.text;
        NSString * savePath = [iNDSEmulationProfile pathForProfileName:self.name];
        [NSKeyedArchiver archiveRootObject:self toFile:savePath];
        [AppDelegate.sharedInstance.currentEmulatorViewController exitEditMode];
        [AppDelegate.sharedInstance.currentEmulatorViewController loadProfile:self];
    }];
    
    [alert showEdit:[AppDelegate sharedInstance].currentEmulatorViewController title:@"Save Profile" subTitle:@"Name for save profile:\n" closeButtonTitle:showCancel ? @"Cancel":nil duration:0.0f];
    
    
}

- (void)updateLayout
{
    mainScreenRects[![self isPortrait]] = self.mainScreen.frame;
    touchScreenRects[![self isPortrait]] = self.touchScreen.frame;
    startButtonRects[![self isPortrait]] = self.startButton.frame;
    selectButtonRects[![self isPortrait]] = self.selectButton.frame;
    leftTriggerRects[![self isPortrait]] = self.leftTrigger.frame;
    rightTriggerRects[![self isPortrait]] = self.rightTrigger.frame;
    directionalControlRects[![self isPortrait]] = self.directionalControl.frame;
    buttonControlRects[![self isPortrait]] = self.buttonControl.frame;
    settingsButtonRects[![self isPortrait]] = self.settingsButton.frame;
    fpsLabelRects[![self isPortrait]] = self.fpsLabel.frame;
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

- (void)enterEditMode
{
    for (UIView * view in [self editableViews]) {
        UIPanGestureRecognizer * pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(pan:)];
        pan.maximumNumberOfTouches = 1;
        [view addGestureRecognizer:pan];
        
        UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
        tap.numberOfTapsRequired = 1;
        tap.numberOfTouchesRequired = 1;
        [view addGestureRecognizer:tap];
        view.frame = [self boundInScreen:view.frame];
    }
    [emulationView.view setNeedsLayout];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didRotate:) name:UIDeviceOrientationDidChangeNotification object:nil];
}

- (void)exitEditMode
{
    for (UIView * view in [self editableViews]) {
        for (UIGestureRecognizer *recognizer in view.gestureRecognizers) {
            [view removeGestureRecognizer:recognizer];
            view.layer.borderWidth = 0;
        }
    }

    [self removeSnapLines];
    [self deselectView];
}

-(NSArray *) editableViews
{
    return @[self.mainScreen, self.touchScreen, self.startButton, self.selectButton, self.leftTrigger, self.rightTrigger, self.directionalControl, self.buttonControl];//, self.fpsLabel]; //Maybe add settings later too
}

- (void) didRotate:(NSNotification *)notification
{
    if (selectedView) {
        [self selectNewView:selectedView];
    }
}

- (void)selectNewView:(UIView *)view;
{
    view.layer.borderWidth = 1;
    //view.layer.borderColor = [UIColor colorWithRed:147/255.0 green:205/255.0 blue:1 alpha:1].CGColor;
    view.layer.borderColor = [UIColor greenColor].CGColor;
    selectedView.layer.borderWidth = 0;
    selectedView = view;
    
    self.sizeSlider.hidden = NO;
    CGSize screenSize = [self currentScreenSizeAlwaysPortrait:NO];
    CGFloat ratio = selectedView.frame.size.height/selectedView.frame.size.width;
    if (screenSize.width * ratio < screenSize.height / ratio) { //Width is limiting this view
        self.sizeSlider.value = selectedView.frame.size.width / screenSize.width;
    } else {
        self.sizeSlider.value = selectedView.frame.size.height / screenSize.height;
    }
    
}

- (void)deselectView
{
    selectedView.layer.borderWidth = 0;
    selectedView = nil;
    self.sizeSlider.hidden = YES;
}

- (void)tap:(UITapGestureRecognizer *)sender
{
    UIView *currentView = sender.view;
    if (currentView != selectedView) {
        [self selectNewView:currentView];
    } else {
        [self deselectView];
    }
}

- (void)sizeChanged:(UISlider *)sender
{
    CGSize screenSize = [self currentScreenSizeAlwaysPortrait:NO];
    
    CGFloat minSize;
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        minSize = 0.1;
    } else { //Allow for smaller buttons on ipad
        minSize = 0.04;
    }
    
    CGRect viewFrame = selectedView.frame;
    CGFloat ratio = viewFrame.size.height/ MAX(viewFrame.size.width, 1);
    if (screenSize.width * ratio < screenSize.height / ratio) { //Width is limiting this view
        viewFrame.size.width = (screenSize.width * MAX(sender.value, minSize));
        viewFrame.size.height = viewFrame.size.width * ratio;
    } else {
        viewFrame.size.height = (screenSize.height * MAX(sender.value, minSize));
        viewFrame.size.width = viewFrame.size.height / ratio;
    }
    //Keep center after editing size
    /*CGPoint oldCenter = selectedView.center;
    selectedView.frame = viewFrame;
    selectedView.center = oldCenter;
    viewFrame = selectedView.frame;*/
    
    selectedView.frame = [self boundInScreen:viewFrame];
    [selectedView setNeedsLayout];
    [self removeSnapLines];
    [self drawSnapLines];
    
    [self updateLayout];
}

- (void)pan:(UIPanGestureRecognizer *)sender
{
    CGPoint location = [(UIPanGestureRecognizer*)sender translationInView:sender.view.superview];
    [self handlePan:sender.view Location:location state:sender.state];
}

- (void)handlePan:(UIView *)currentView Location:(CGPoint) location state:(UIGestureRecognizerState) state
{
    CGPoint translatedPoint = location;
    NSArray * views = @[self.mainScreen, self.touchScreen, self.startButton, self.selectButton, self.leftTrigger, self.rightTrigger, self.directionalControl, self.buttonControl];
    if (state == UIGestureRecognizerStateBegan) {
        offsetX = translatedPoint.x - currentView.frame.origin.x;
        offsetY = translatedPoint.y - currentView.frame.origin.y;
        if (currentView != selectedView) {
            [self selectNewView:currentView];
        }
    }
    if (state == UIGestureRecognizerStateChanged) {
        [self removeSnapLines];
        CGRect viewFrame = currentView.frame;
        
        //Snap to other views
        
        viewFrame.origin.x = translatedPoint.x - offsetX;
        viewFrame.origin.y = translatedPoint.y - offsetY;
        
        currentView.frame = viewFrame;
        
        CGRect destinationFrame = viewFrame;
        CGFloat minSnapLength = 6;
        for (UIView * otherView in views) {
            if (otherView != currentView) {
                otherView.layer.borderWidth = 0;
                //Snap X
                if (fabs(viewFrame.origin.x - otherView.frame.origin.x) < minSnapLength) {
                    minSnapLength = fabs(viewFrame.origin.x - otherView.frame.origin.x);
                    destinationFrame.origin.x = otherView.frame.origin.x;
                }
                if (fabs(viewFrame.origin.x - (otherView.frame.origin.x + otherView.frame.size.width)) < minSnapLength) {
                    minSnapLength = fabs(viewFrame.origin.x - (otherView.frame.origin.x + otherView.frame.size.width));
                    destinationFrame.origin.x = (otherView.frame.origin.x + otherView.frame.size.width);
                }
                if (fabs((viewFrame.origin.x + viewFrame.size.width) - otherView.frame.origin.x) < minSnapLength) {
                    minSnapLength = fabs((viewFrame.origin.x + viewFrame.size.width) - otherView.frame.origin.x);
                    destinationFrame.origin.x = otherView.frame.origin.x - viewFrame.size.width;
                }
                if (fabs((viewFrame.origin.x + viewFrame.size.width) - (otherView.frame.origin.x + otherView.frame.size.width)) < minSnapLength) {
                    minSnapLength = fabs((viewFrame.origin.x + viewFrame.size.width) - (otherView.frame.origin.x + otherView.frame.size.width));
                    destinationFrame.origin.x = otherView.frame.origin.x + otherView.frame.size.width - viewFrame.size.width;
                }
                
                //Snap Y
                if (fabs(viewFrame.origin.y - otherView.frame.origin.y) < minSnapLength) {
                    minSnapLength = fabs(viewFrame.origin.y - otherView.frame.origin.y);
                    destinationFrame.origin.y = otherView.frame.origin.y;
                }
                if (fabs(viewFrame.origin.y - (otherView.frame.origin.y + otherView.frame.size.height)) < minSnapLength) {
                    minSnapLength = fabs(viewFrame.origin.y - (otherView.frame.origin.y + otherView.frame.size.height));
                    destinationFrame.origin.y = (otherView.frame.origin.y + otherView.frame.size.height);
                }
                if (fabs((viewFrame.origin.y + viewFrame.size.height) - otherView.frame.origin.y) < minSnapLength) {
                    minSnapLength = fabs((viewFrame.origin.y + viewFrame.size.height) - otherView.frame.origin.y);
                    destinationFrame.origin.y = otherView.frame.origin.y - viewFrame.size.height;
                }
                if (fabs((viewFrame.origin.y + viewFrame.size.height) - (otherView.frame.origin.y + otherView.frame.size.height)) < minSnapLength) {
                    minSnapLength = fabs((viewFrame.origin.y + viewFrame.size.height) - (otherView.frame.origin.y + otherView.frame.size.height));
                    destinationFrame.origin.y = otherView.frame.origin.y + otherView.frame.size.height - viewFrame.size.height;
                }
            }
        }
        viewFrame = destinationFrame;
        
        currentView.frame = [self boundInScreen:viewFrame];
        
        // Draw snap lines after snapping is finished
        [self drawSnapLines];
    }
    
    if (state == UIGestureRecognizerStateEnded) {
        for (UIView * otherView in views) {
            if (otherView != currentView) {
                otherView.layer.borderWidth = 0;
            }
        }
        [self removeSnapLines];
        [self updateLayout];
    }
}

- (CGRect)boundInScreen:(CGRect)viewFrame
{
    CGSize screenSize = [self currentScreenSizeAlwaysPortrait:NO];
    if (viewFrame.origin.x < 0) {
        viewFrame.origin.x = 0;
    } else if (viewFrame.origin.x + viewFrame.size.width > screenSize.width) {
        viewFrame.origin.x = screenSize.width - viewFrame.size.width;
    }
    
    if (viewFrame.origin.y < 0) {
        viewFrame.origin.y = 0;
    } else if (viewFrame.origin.y + viewFrame.size.height > screenSize.height) {
        viewFrame.origin.y = screenSize.height - viewFrame.size.height;
    }
    return viewFrame;
}

- (void)drawSnapLines
{
    CGRect viewFrame = selectedView.frame;
    for (UIView * otherView in [self editableViews]) {
        if (otherView != selectedView) {
            if (viewFrame.origin.x == otherView.frame.origin.x) {
                [self drawSnapLineAtPosition:viewFrame.origin.x Direction:VERTICAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if (viewFrame.origin.x == (otherView.frame.origin.x + otherView.frame.size.width)) {
                [self drawSnapLineAtPosition:viewFrame.origin.x Direction:VERTICAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if ((viewFrame.origin.x + viewFrame.size.width) == otherView.frame.origin.x) {
                [self drawSnapLineAtPosition:viewFrame.origin.x + viewFrame.size.width Direction:VERTICAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if ((viewFrame.origin.x + viewFrame.size.width) == (otherView.frame.origin.x + otherView.frame.size.width)) {
                [self drawSnapLineAtPosition:viewFrame.origin.x + viewFrame.size.width Direction:VERTICAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            
            if (viewFrame.origin.y == otherView.frame.origin.y) {
                [self drawSnapLineAtPosition:viewFrame.origin.y Direction:HORIZONTAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if (viewFrame.origin.y == (otherView.frame.origin.y + otherView.frame.size.height)) {
                [self drawSnapLineAtPosition:viewFrame.origin.y Direction:HORIZONTAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if ((viewFrame.origin.y + viewFrame.size.height) == otherView.frame.origin.y) {
                [self drawSnapLineAtPosition:viewFrame.origin.y + viewFrame.size.height Direction:HORIZONTAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
            if ((viewFrame.origin.y + viewFrame.size.height) == (otherView.frame.origin.y + otherView.frame.size.height)) {
                [self drawSnapLineAtPosition:viewFrame.origin.y + viewFrame.size.height Direction:HORIZONTAL];
                otherView.layer.borderWidth = 1;
                otherView.layer.borderColor = [UIColor yellowColor].CGColor;
            }
        }
    }
}

- (void)removeSnapLines
{
    NSMutableArray * layersToRemove = [NSMutableArray new];
    for (CALayer * layer in emulationView.gameContainer.layer.sublayers) {
        if ([layer.name isEqualToString:@"Snap"]) {
            [layersToRemove addObject:layer];
        }
    }
    for (CALayer * layer in layersToRemove) {
        [layer removeFromSuperlayer];
    }
}

- (void)drawSnapLineAtPosition:(NSInteger)positon Direction:(NSInteger)direction
{
    CALayer * snapLine = [[CALayer alloc] init];
    snapLine.backgroundColor = [UIColor redColor].CGColor;
    if (direction == VERTICAL)
        snapLine.frame = CGRectMake(positon, 0, 1, emulationView.view.frame.size.height);
    else
        snapLine.frame = CGRectMake(0, positon, emulationView.view.frame.size.width,  1);
    snapLine.zPosition = 100;
    snapLine.name = @"Snap";
    
    [emulationView.gameContainer.layer addSublayer:snapLine];
}

-(CGSize)currentScreenSizeAlwaysPortrait:(BOOL)portrait
{
    if (!portrait)
        return [UIScreen mainScreen].bounds.size;
    //Get portrait size
    CGRect screenBounds = [UIScreen mainScreen].bounds ;
    CGFloat width = CGRectGetWidth(screenBounds);
    CGFloat height = CGRectGetHeight(screenBounds);
    if ([self isPortrait]){
        return CGSizeMake(width, height);
    }
    return CGSizeMake(height, width);
}

-(BOOL) isPortrait
{
    return UIInterfaceOrientationIsPortrait([UIApplication sharedApplication].statusBarOrientation);
}

+ (NSString*)pathForProfileName:(NSString *)name
{
    return [[AppDelegate.sharedInstance.batteryDir stringByAppendingPathComponent:name] stringByAppendingPathExtension:@"ips"];
}

- (BOOL)deleteProfile
{
    NSString * path = [iNDSEmulationProfile pathForProfileName:self.name];
    if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
        [CHBgDropboxSync forceStopIfRunning];
        NSError *error;
        BOOL success = [[NSFileManager defaultManager] removeItemAtPath:path error:&error];
        if (!success) {
            NSLog(@"Could not delete file :%@ ", error);
            return NO;
        }
        return YES;
    }
    NSLog(@"No file! %@", path);
    return NO;
}

@end

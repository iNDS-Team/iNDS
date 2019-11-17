//
//  iNDSEmulatorViewController.m
//  iNDS
//
//  Created by iNDS on 6/11/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//


#import "AppDelegate.h"
#import "iNDSEmulatorViewController.h"
#import "GLProgram.h"
#import "UIScreen+Widescreen.h"
#import "iNDSDirectionalControl.h"
#import "iNDSButtonControl.h"
#import "CHBgDropboxSync.h"

#import "UIDevice+Private.h"
#import "RBVolumeButtons.h"

#import <GLKit/GLKit.h>
#import <OpenGLES/ES2/gl.h>
#import <AudioToolbox/AudioToolbox.h>
#import <GameController/GameController.h>

#include "emu.h"
//#include "mic.h"
#import "SCLAlertView.h"

#import "iNDSMFIControllerSupport.h"
#import "iNDSEmulationProfile.h"

#import "iCadeReaderView.h"

#import <AVFoundation/AVFoundation.h>

#define STRINGIZE(x) #x
#define STRINGIZE2(x) STRINGIZE(x)
#define SHADER_STRING(text) @ STRINGIZE2(text)
#define ICADE_BTN_NUMBER(button) [NSNumber numberWithInt:button]

NSDictionary *iCadeMapping;

NSString *const kVertShader = SHADER_STRING
(
 attribute vec4 position;
 attribute vec2 inputTextureCoordinate;
 
 varying highp vec2 texCoord;
 
 void main()
 {
     texCoord = inputTextureCoordinate;
     gl_Position = position;
 }
 );
NSString * kFragShader = SHADER_STRING (
 uniform sampler2D inputImageTexture;
 varying highp vec2 texCoord;
 
 void main()
 {
     highp vec4 color = texture2D(inputImageTexture, texCoord);
     gl_FragColor = color;
 }
 );

const float positionVert[] =
{
    -1.0f, 1.0f,
    1.0f, 1.0f,
    -1.0f, -1.0f,
    1.0f, -1.0f
};

const float textureVert[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

enum VideoFilter : NSUInteger {
    NONE,
    HQ2X,
    _2XSAI,
    SUPER2XSAI,
    SUPEREAGLE,
    SCANLINE,
    BILINEAR,
    NEAREST2X,
    HQ2XS,
    LQ2X,
    LQ2XS,
    EPX,
    NEARESTPLUS1POINT5,
    NEAREST1POINT5,
    EPXPLUS,
    EPX1POINT5,
    EPXPLUS1POINT5,
    HQ4X,
    BRZ2x,
    BRZ3x,
    BRZ4x,
    BRZ5x
};

@interface iNDSEmulatorViewController () <GLKViewDelegate, iCadeEventDelegate> {
    float coreFps;
    float videoFps;
    
    GLuint texHandle[2];
    GLint attribPos;
    GLint attribTexCoord;
    GLint texUniform;
    
    GLKView *glkView[2];
    GLKView *movingView;
    
    iNDSButtonControlButton _previousButtons;
    iNDSDirectionalControlDirection _previousDirection;
    
    NSLock *emuLoopLock;
    
    UIWindow *extWindow;
    
    CFTimeInterval lastAutosave;
    
    BOOL settingsShown;
    BOOL inEditingMode;
    BOOL disableTouchScreen;
    
    UINavigationController * settingsNav;
    
    RBVolumeButtons *volumeStealer;
    
    CADisplayLink *coreLink;
    dispatch_semaphore_t displaySemaphore;
    
    UIFeedbackGenerator *vibration;
}


@property (weak, nonatomic) IBOutlet UILabel *fpsLabel;
@property (strong, nonatomic) GLProgram *program;
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) IBOutlet UIView *controllerContainerView;
@property (strong, nonatomic) IBOutlet UISlider *sizeSlider;

@property (weak, nonatomic) IBOutlet iNDSDirectionalControl *directionalControl;
@property (weak, nonatomic) IBOutlet iNDSButtonControl *buttonControl;
@property (weak, nonatomic) IBOutlet UIButton *settingsButton;
@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UIButton *selectButton;
@property (weak, nonatomic) IBOutlet UIButton *leftTrigger;
@property (weak, nonatomic) IBOutlet UIButton *rightTrigger;
@property (strong, nonatomic) UIImageView *snapshotView;

- (IBAction)onButtonUp:(UIControl*)sender;
- (IBAction)onButtonDown:(UIControl*)sender;

@end

@implementation iNDSEmulatorViewController

#pragma mark - UIViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    NSString * currentProfile = [[NSUserDefaults standardUserDefaults] stringForKey:@"currentProfile"];
    iNDSEmulationProfile * profile;
    if ([currentProfile isEqualToString:@"iNDSDefaultProfile"]) {
        profile = [[iNDSEmulationProfile alloc] initWithProfileName:@"iNDSDefaultProfile"];
    } else {
        profile = [iNDSEmulationProfile profileWithPath:[iNDSEmulationProfile pathForProfileName:currentProfile]];
        if (!profile) {
            profile = [[iNDSEmulationProfile alloc] initWithProfileName:@"iNDSDefaultProfile"];
        }
    }
    [self loadProfile:profile];
    
    self.view.multipleTouchEnabled = YES;
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self selector:@selector(suspendEmulation) name:UIApplicationWillResignActiveNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(resumeEmulation) name:UIApplicationDidBecomeActiveNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(defaultsChanged:) name:NSUserDefaultsDidChangeNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(screenChanged:) name:UIScreenDidConnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(screenChanged:) name:UIScreenDidDisconnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(controllerActivated:) name:GCControllerDidConnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(controllerDeactivated:) name:GCControllerDidDisconnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(userRequestedToPlayROM:) name:iNDSUserRequestedToPlayROMNotification object:nil];

    if ([[GCController controllers] count] > 0) {
        [self controllerActivated:nil];
    }
    
    // Volume Button Bumpers
    // This works pretty well but there's a 0.5 second delay when
    // the user holds down a volume button
    // If we could intercept the actual event instead of just listening for the notification, timing would be a lot more accurate
    volumeStealer = [[RBVolumeButtons alloc] init];
    __block CFTimeInterval lastButtonUp = 0;
    __block CFTimeInterval lastButtonDown = 0;
    volumeStealer.upBlock = ^{
        EMU_buttonDown((BUTTON_ID)11); //R
        lastButtonUp = CACurrentMediaTime();
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 0.18), dispatch_get_main_queue(), ^(void){
            if (CACurrentMediaTime() > lastButtonUp + 0.1) {//Another volume button intercept hasn't fired
                EMU_buttonUp((BUTTON_ID)11);
            }
        });
    };
    volumeStealer.downBlock = ^{
        EMU_buttonDown((BUTTON_ID)10); //L
        lastButtonDown = CACurrentMediaTime();
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 0.18), dispatch_get_main_queue(), ^(void){
            if (CACurrentMediaTime() > lastButtonDown + 0.1) {
                EMU_buttonUp((BUTTON_ID)10);
            }
        });
    };
    
    
    [self defaultsChanged:nil];
    
    _speed = 1;

    
    CGRect settingsRect = self.settingsContainer.frame;
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
        settingsRect = self.view.frame;
    } else {
        settingsRect.size.width = MIN(500, self.view.frame.size.width - 70);
        settingsRect.size.height = MIN(600, self.view.frame.size.height - 140);
        self.settingsContainer.layer.cornerRadius = 7;
    }
    self.settingsContainer.frame = settingsRect;
    self.settingsContainer.center = self.view.center;
    self.settingsContainer.subviews[0].frame = self.settingsContainer.bounds; //Set the inside view
    
    // ICade
    iCadeReaderView *control = [[iCadeReaderView alloc] initWithFrame:CGRectZero];
    [self.view addSubview:control];
    control.active = YES;
    control.delegate = self;
    [self setICadeMapping];
    
    disableTouchScreen = [[NSUserDefaults standardUserDefaults] boolForKey:@"disableTouchScreen"];
    
    coreLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(emulatorLoop)];
    if (@available(iOS 10, *)) {
        [coreLink setPreferredFramesPerSecond:60];
    }    
    coreLink.paused = YES;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        [coreLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] run];
    });
    
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self pauseEmulation];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    if (!settingsShown)
        [self loadROM];
    [self defaultsChanged:nil];
    [self.profile ajustLayout];
}

- (void)changeGame:(iNDSGame *)newGame
{
    NSLog(@"Changing Game");
    [self saveStateWithName:@"Pause"];
    [self suspendEmulation];
    //EMU_closeRom();
    self.game = newGame;
    [self loadROM];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.profile ajustLayout];
        [self toggleSettings:self];
    });
}

- (void) userRequestedToPlayROM:(NSNotification *) notification {
    NSLog(@"Notification");
    iNDSGame *game = notification.object;
    
    if ([self.game isEqual:game]) {
        return;
    }
    
    if (self.game == nil) {
        [AppDelegate.sharedInstance startGame:game withSavedState:-1];
    }
    
    [self pauseEmulation];
    
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Game Currently Running" message:[NSString stringWithFormat:@"Would you like to end %@ and start %@? All unsaved data will be lost.", self.game.title, game.title] preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * action) {
        [self resumeEmulation];
    }];
    UIAlertAction *continueAction = [UIAlertAction actionWithTitle:@"Continue" style:UIAlertActionStyleDestructive handler:^(UIAlertAction * _Nonnull action) {
        [self changeGame:game];
    }];
    
    [alert addAction:cancelAction];
    [alert addAction:continueAction];
    
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden
{
    return !settingsShown;
}

- (void)loadProfile:(iNDSEmulationProfile *)profile
{
    self.profile = profile;
    [[NSUserDefaults standardUserDefaults] setObject:profile.name forKey:@"currentProfile"];
    //Attach views
    self.profile.directionalControl = self.directionalControl;
    self.profile.buttonControl = self.buttonControl;
    self.profile.settingsButton = self.settingsButton;
    self.profile.startButton = self.startButton;
    self.profile.selectButton = self.selectButton;
    self.profile.leftTrigger = self.leftTrigger;
    self.profile.rightTrigger = self.rightTrigger;
    self.profile.fpsLabel = self.fpsLabel;
    self.profile.mainScreen = glkView[0];
    self.profile.touchScreen = glkView[1];
    self.profile.sizeSlider = self.sizeSlider;
    [self.sizeSlider addTarget:self.profile action:@selector(sizeChanged:) forControlEvents:UIControlEventValueChanged];
    
}

- (void)defaultsChanged:(NSNotification*)notification
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (emuLoopLock) { //Only update these is the core has loaded
        EMU_setFrameSkip((int)[defaults integerForKey:@"frameSkip"]);
        EMU_setSynchMode([defaults boolForKey:@"synchSound"]);
        
        // (Mute on && don't ignore it) or user has sound disabled
        BOOL muteSound = [defaults boolForKey:@"disableSound"];
        EMU_enableSound(!muteSound);
        AVAudioSessionCategoryOptions opts = AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetoothA2DP;
        if([defaults boolForKey:@"allowExternalAudio"]){
            opts |= AVAudioSessionCategoryOptionMixWithOthers;
        }
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:opts error:nil];
        
        // Filter
        int filterTranslate[] = {NONE, EPX, SUPEREAGLE, _2XSAI, SUPER2XSAI, BRZ2x, LQ2X, BRZ3x, HQ2X, HQ4X, BRZ4x, BRZ5x};
        NSInteger filter = [[NSUserDefaults standardUserDefaults] integerForKey:@"videoFilter"];
        EMU_setFilter(filterTranslate[filter]);
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        self.directionalControl.style = [defaults integerForKey:@"controlPadStyle"];
        self.fpsLabel.hidden = ![defaults integerForKey:@"showFPS"];
    });
    
    
    // For some reason both of these disable the mic
    if ([defaults integerForKey:@"volumeBumper"] && !settingsShown) {
        [volumeStealer startStealingVolumeButtonEvents];
    } else {
        [volumeStealer stopStealingVolumeButtonEvents];
    }
    
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 1) {
//        [self vibrateSoft];
        vibration = [[UISelectionFeedbackGenerator alloc] init];
    } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 2) {
        vibration = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    }
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.view setNeedsLayout];
    });
}


-(BOOL) isPortrait
{
    return UIInterfaceOrientationIsPortrait([UIApplication sharedApplication].statusBarOrientation);
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    self.gameContainer.frame = self.view.frame;
    self.controllerContainerView.frame = self.view.frame;
    [self.profile ajustLayout];
    CGRect settingsRect = self.settingsContainer.frame;
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
        settingsRect = self.view.frame;
        self.settingsContainer.layer.cornerRadius = 0;
    } else {
        if ([self isPortrait]) { //Portrait
            settingsRect.size.width = MIN(500, self.view.frame.size.width - 70);
            settingsRect.size.height = MIN(600, self.view.frame.size.height - 140);
        } else {
            settingsRect.size.width = MIN(500, self.view.frame.size.width - 70);
            settingsRect.size.height = MIN(600, self.view.frame.size.height - 60);
        }
        self.settingsContainer.layer.cornerRadius = 7;
    }
    
    
    self.settingsContainer.frame = settingsRect;
    self.settingsContainer.center = self.view.center;
    self.settingsContainer.subviews[0].frame = self.settingsContainer.bounds; //Set the inside view
    
    self.controllerContainerView.alpha = [[NSUserDefaults standardUserDefaults] floatForKey:@"controlOpacity"];
    self.startButton.alpha = [[NSUserDefaults standardUserDefaults] floatForKey:@"controlOpacity"];
    self.selectButton.alpha = [[NSUserDefaults standardUserDefaults] floatForKey:@"controlOpacity"];
    if ([UIScreen screens].count > 1) {
        CGSize screenSize = [UIScreen screens][1].bounds.size;
        CGSize viewSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333), MIN(screenSize.width, screenSize.height * 1.333) * 0.75);
        glkView[0].frame = CGRectMake(screenSize.width/2 - viewSize.width/2, screenSize.height/2 - viewSize.height/2, viewSize.width, viewSize.height);
    }
}


- (void)dealloc
{
    EMU_closeRom();
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)screenChanged:(NSNotification*)notification
{
    [self pauseEmulation];
    if ([UIScreen screens].count > 1) {
        UIScreen *extScreen = [UIScreen screens][1];
        extScreen.currentMode = extScreen.availableModes[0];
        extWindow = [[UIWindow alloc] initWithFrame:extScreen.bounds];
        extWindow.screen = extScreen;
        extWindow.backgroundColor = [UIColor orangeColor];
        [extWindow addSubview:glkView[0]];
        [extWindow makeKeyAndVisible];
    } else {
        extWindow = nil;
        [self.gameContainer insertSubview:glkView[0] atIndex:0];
    }
    [self.view setNeedsLayout];
    [self performSelector:@selector(resumeEmulation) withObject:nil afterDelay:0.5];
}

#pragma mark - Playing ROM



- (void)loadROM {
    NSLog(@"Loading ROM %@", self.game.path);
    EMU_setWorkingDir([[self.game.path stringByDeletingLastPathComponent] fileSystemRepresentation]);
    EMU_init([iNDSGame preferredLanguage]);
    //2 for JIT
    EMU_setCPUMode((int)[[NSUserDefaults standardUserDefaults] integerForKey:@"cpuMode"] + 1);
    EMU_setAdvancedBusTiming([[NSUserDefaults standardUserDefaults] boolForKey:@"adv_timing"]);
    EMU_setDepthComparisonThreshold([[NSUserDefaults standardUserDefaults] boolForKey:@"depth"]);
    EMU_loadRom([self.game.path fileSystemRepresentation]);
    EMU_change3D(1);
        
    [self initGL];
    
    emuLoopLock = [NSLock new];
    
    if (self.saveState) EMU_loadState(self.saveState.fileSystemRepresentation);
    [self startEmulatorLoop];
    [self defaultsChanged:nil];
}

- (void)initGL
{
    NSLog(@"Initiaizing GL");
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:self.context];
    //self.context.multiThreaded = YES;
    NSLog(@"Is multi: %d", self.context.isMultiThreaded);
    
    if ([UIScreen screens].count > 1) {
        UIScreen *extScreen = [UIScreen screens][1];
        extScreen.currentMode = extScreen.availableModes[0];
        extWindow = [[UIWindow alloc] initWithFrame:extScreen.bounds];
        extWindow.screen = extScreen;
        extWindow.backgroundColor = [UIColor orangeColor];
        CGSize screenSize = [UIScreen screens][1].bounds.size;
        CGSize viewSize = CGSizeMake(MIN(screenSize.width, screenSize.height * 1.333), MIN(screenSize.width, screenSize.height * 1.333) * 0.75);
        CGRect mainScreenRect = CGRectMake(screenSize.width/2 - viewSize.width/2, screenSize.height/2 - viewSize.height/2, viewSize.width, viewSize.height);
        glkView[0] = [[GLKView alloc] initWithFrame:mainScreenRect context:self.context];
        glkView[1] = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context]; //4:3 ratio is all that matters
        glkView[0].delegate = self;
        glkView[1].delegate = self;
        [self.gameContainer insertSubview:glkView[1] atIndex:0];
        [extWindow addSubview:glkView[0]];
        [extWindow makeKeyAndVisible];
    } else { //1 screen
        glkView[0] = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
        glkView[1] = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
        glkView[0].delegate = self;
        glkView[1].delegate = self;
        [self.gameContainer insertSubview:glkView[1] atIndex:0];
        [self.gameContainer insertSubview:glkView[0] atIndex:0];
    }
    
    //Attach views to profile
    self.profile.mainScreen = glkView[0];
    self.profile.touchScreen = glkView[1];
    
    kFragShader = SHADER_STRING (
                       uniform sampler2D inputImageTexture;
                       varying highp vec2 texCoord;
                       
                       void main()
                       {
                           highp vec4 color = texture2D(inputImageTexture, texCoord);
                           gl_FragColor = color;
                       }
                       );
    
    
    self.program = [[GLProgram alloc] initWithVertexShaderString:kVertShader fragmentShaderString:kFragShader];
    
    [self.program addAttribute:@"position"];
	[self.program addAttribute:@"inputTextureCoordinate"];
    
    [self.program link];
    
    attribPos = [self.program attributeIndex:@"position"];
    attribTexCoord = [self.program attributeIndex:@"inputTextureCoordinate"];
    
    texUniform = [self.program uniformIndex:@"inputImageTexture"];
    
    glEnableVertexAttribArray(attribPos);
    glEnableVertexAttribArray(attribTexCoord);
    
    glViewport(0, 0, 4, 3);//size.width, size.height);
    
    [self.program use];
    
    glGenTextures(2, texHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texHandle[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    //Window 2
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

- (void)shutdownGL
{
    glDeleteTextures(2, texHandle);
    texHandle[0] = 0;
    texHandle[1] = 0;
    self.context = nil;
    self.program = nil;
    [glkView[0] removeFromSuperview];
    [glkView[1] removeFromSuperview];
    glkView[0] = glkView[1] = nil;
    [EAGLContext setCurrentContext:nil];
    extWindow = nil;
}

- (UIImage*)screenSnapshot:(NSInteger)num
{
    UIGraphicsBeginImageContextWithOptions(self.view.bounds.size, NO, [UIScreen mainScreen].scale);
    
    [self.view drawViewHierarchyInRect:self.view.bounds afterScreenUpdates:YES];
    
    UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return image;
}

- (void)suspendEmulation
{
    [self setLidClosed:YES];
    NSLog(@"Suspending");
    [self pauseEmulation];
    // Shutting down while editing a layout causes a ton of problems.
    // So We'll just not shutdown while editing... :/
    if (!inEditingMode) { //Discard the changes
        [self shutdownGL];
    }
}

- (void)pauseEmulation
{
    NSLog(@"Pausing");
    if (!execute) return;
    EMU_pause(true);
    [emuLoopLock lock]; // make sure emulator loop has ended
    [emuLoopLock unlock];
}

- (void)resumeEmulation
{
    NSLog(@"Resuming");
    if (self.presentingViewController.presentedViewController != self) return;
    if (execute) return;
    
    if (!self.program){
        NSLog(@"Resuming From Suspend");
        [self initGL];
        [self.view setNeedsLayout];
    }
    
    if (inEditingMode || settingsShown) return; //Rebuild GL but don't start just yet
    
    EMU_pause(false);
    //[self.profile ajustLayout];
    [self startEmulatorLoop];
}

- (void)startEmulatorLoop
{
    //[self.view endEditing:YES];
    [self updateDisplay]; //This has to be called once before we touch or move any glk views
    
    displaySemaphore = dispatch_semaphore_create(1);
    
    lastAutosave = CACurrentMediaTime();
    [emuLoopLock lock];
    [[iNDSMFIControllerSupport instance] startMonitoringGamePad];
    
    coreFps = videoFps = 30;
    
    coreLink.paused = NO;
}

- (void)endEmulatorLoop
{
    NSLog(@"Ending loop");
    [[iNDSMFIControllerSupport instance] stopMonitoringGamePad];
    [emuLoopLock unlock];
    coreLink.paused = YES;
}

//Relocate
CFTimeInterval coreStart, loopStart;
CGFloat framesToRender = 0;
NSInteger filter = [[NSUserDefaults standardUserDefaults] integerForKey:@"videoFilter"];

- (void)emulatorLoop
{
    if (!execute) {
        [self endEmulatorLoop];
    }
    loopStart = CACurrentMediaTime();
    // Running under 60 fps makes the sound terrible
    // so going a little over isn't bad
    framesToRender += self.speed + 0.02;// ~61 fps.
    NSInteger framesRendered = 0;
    for (;framesToRender >= 1; framesToRender--, framesRendered++) {
        if (framesToRender >= 2) {
            EMU_frameSkip(true);
        }
        coreStart = CACurrentMediaTime();
        EMU_runCore();
        coreFps = coreFps * 0.99 + (1 / (CACurrentMediaTime() - coreStart)) * 0.01;
    }
    
    if (CACurrentMediaTime() - lastAutosave > 180) {
        CGFloat coreTime = [[NSUserDefaults standardUserDefaults] floatForKey:@"coreTime"];
        coreTime = coreTime * 0.95 + (CACurrentMediaTime() - coreStart) * 0.05;
        [[NSUserDefaults standardUserDefaults] setFloat:coreTime forKey:@"coreTime"];
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"periodicSave"]) {
            [self saveStateWithName:[NSString stringWithFormat:@"Auto Save"]];
        }
        lastAutosave = CACurrentMediaTime();
        filter = [[NSUserDefaults standardUserDefaults] integerForKey:@"videoFilter"];
    }
    
    if (!EMU_frameSkip(false)) {
        [self calculateFPS:1];
        EMU_copyMasterBuffer();
        if (filter == -1) {
            [self updateDisplay];
        } else {
            // Run the filter on a seperate thread to increase performance
            // Core will always be one frame ahead
            dispatch_semaphore_wait(displaySemaphore, DISPATCH_TIME_FOREVER);
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                //This will automatically throttle fps to 60
                [self updateDisplay];
                dispatch_semaphore_signal(displaySemaphore);
            });
        }
    }
}

- (void)calculateFPS:(NSInteger) frames
{
    static CFTimeInterval lastDisplayTime = 0;
    videoFps = videoFps * 0.95 + (frames / (CACurrentMediaTime() - lastDisplayTime)) * 0.05;
    lastDisplayTime = CACurrentMediaTime();
    
    
    static CFTimeInterval fpsUpdateTime = 0;
    if (CACurrentMediaTime() - fpsUpdateTime > 1) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (IsiPhoneX) {
                self.fpsLabel.text = [NSString stringWithFormat:@"FPS %d\nMax Core %d", MIN((int)videoFps, 60), (int)(coreFps / self.speed)];
            } else {
                self.fpsLabel.text = [NSString stringWithFormat:@"FPS %d Max Core %d", MIN((int)videoFps, 60), (int)(coreFps / self.speed)];
            }
        });
        fpsUpdateTime = CACurrentMediaTime();
    }
}

- (void)saveStateWithName:(NSString*)saveStateName
{
    NSLog(@"Saving: %@", saveStateName);
    NSString *savePath = [self.game pathForSaveStateWithName:saveStateName];
    if ([[NSFileManager defaultManager] fileExistsAtPath:savePath]) {
        [[NSFileManager defaultManager] removeItemAtPath:savePath error:nil];
    }
    EMU_saveState([self.game pathForSaveStateWithName:saveStateName].fileSystemRepresentation);
    [self.game reloadSaveStates];
}

- (void)updateDisplay
{
    if (texHandle[0] == 0 || !execute) return;
    
    size_t bufferSize;
    GLubyte *screenBuffer = (GLubyte*)EMU_getVideoBuffer(&bufferSize);
    
    int scale = (int)sqrt(bufferSize/98304);//1, 3, 4
    
    int width = 256 * scale;
    int height = 192 * scale;
    
    glBindTexture(GL_TEXTURE_2D, texHandle[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);
    [glkView[0] display]; // This will automatically throttle to 60 fps
    
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer + width * height * 4);
    [glkView[1] display];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (view == glkView[0]) ? texHandle[0] : texHandle[1]);
    glUniform1i(texUniform, 1);
    
    glVertexAttribPointer(attribPos, 2, GL_FLOAT, 0, 0, (const GLfloat*)&positionVert);
    glVertexAttribPointer(attribTexCoord, 2, GL_FLOAT, 0, 0, (const GLfloat*)&textureVert);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#pragma mark - API

- (void)enterEditMode
{
    inEditingMode = YES;
    settingsShown = YES;
    [self toggleSettings:self];
    [self pauseEmulation];
    [self.profile enterEditMode];
}

- (void)exitEditMode
{
    [self.profile exitEditMode];
    inEditingMode = NO;
    settingsShown = YES;
    [self toggleSettings:self];
    [settingsNav popToRootViewControllerAnimated:YES];
    
}

- (IBAction)toggleSettings:(id)sender
{
    if (!settingsShown) { //About to show settings
        [self setLidClosed:YES];
        // Give time for lid close
        //dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [volumeStealer performSelector:@selector(stopStealingVolumeButtonEvents) withObject:nil afterDelay:0.1]; //Non blocking
            [self.settingsContainer setHidden:NO];
            [self pauseEmulation];
            [UIView animateWithDuration:0.3 animations:^{
                self.darkenView.hidden = NO;
                self.darkenView.alpha = 0.6;
                self.settingsContainer.alpha = 1;
                settingsShown = YES;
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
                    [self setNeedsStatusBarAppearanceUpdate];
                }
            } completion:^(BOOL finished) {
                if (!inEditingMode)
                    [CHBgDropboxSync start];
            }];
        //});
        
    } else {
        
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"volumeBumper"]) {
            [volumeStealer performSelector:@selector(startStealingVolumeButtonEvents) withObject:nil afterDelay:0.1];
        }
        [UIView animateWithDuration:0.3 animations:^{
            self.darkenView.alpha = 0.0;
            self.settingsContainer.alpha = 0;
            settingsShown = NO;
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
                [self setNeedsStatusBarAppearanceUpdate];
            }
        } completion:^(BOOL finished) {
            self.settingsContainer.hidden = YES;
            [self resumeEmulation];
            self.darkenView.hidden = YES;
        }];
        
        disableTouchScreen = [[NSUserDefaults standardUserDefaults] boolForKey:@"disableTouchScreen"];
        
        [self setLidClosed:NO];
    }
    
}

- (void) setSettingsHeight:(CGFloat) height
{
    [UIView animateWithDuration:0.3 animations:^{
        CGRect settingsRect = self.settingsContainer.frame;
        settingsRect.size.height = MAX(MIN(height, self.view.frame.size.height - 100), 300);
        self.settingsContainer.frame = settingsRect;
        self.settingsContainer.center = self.view.center;
        self.settingsContainer.subviews[0].frame = self.settingsContainer.bounds; //Set the inside view
    }];
    
    
}

- (UIView *)statuBarView
{
    NSString *key = @"statusBar";
    id object = [UIApplication sharedApplication];
    UIView *statusBar = nil;
    if ([object respondsToSelector:NSSelectorFromString(key)]) {
        statusBar = [object valueForKey:key];
    }
    return statusBar;
}

#pragma mark - Saving

- (void)newSaveState
{
    [self pauseEmulation];
    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    
    UITextField *textField = [alert addTextField:@""];
    
    [alert addButton:@"Save" actionBlock:^(void) {
        [self saveStateWithName:textField.text];
        [self toggleSettings:self];
    }];
    
    [alert addButton:@"Cancel" actionBlock:^(void) {
        //[self toggleSettings:self];
    }];
    
    [alert showEdit:self title:@"Save State" subTitle:@"Name for save state:\n" closeButtonTitle:nil duration:0.0f];
}

- (void)reloadEmulator
{
    NSLog(@"Reloading");
    [self pauseEmulation];
    [self shutdownGL];
    [self loadROM];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.profile ajustLayout];
        [self toggleSettings:self];
    });
    [self saveStateWithName:@"iNDSReloadState"];
    //[self setLidClosed:NO];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([segue.identifier isEqualToString:@"SettingsEmbed"]) {
        settingsNav = segue.destinationViewController;
    }
}

#pragma mark - Controls

- (void) setSpeed:(CGFloat)speed
{
    if (!self.program) return;
    if (speed < 1) { //Force this. Audio sounds so bad < 1 when synched
        EMU_setSynchMode(0);
    } else {
        EMU_setSynchMode([[NSUserDefaults standardUserDefaults] boolForKey:@"synchSound"]);
    }
    _speed = speed;
}

- (void)setLidClosed:(BOOL)closed
{
    return; //Disabled until freezing is fixed
    if (closed) {
        EMU_buttonDown((BUTTON_ID)13);
    } else {
        EMU_buttonUp((BUTTON_ID)13);
    }
}

- (void)controllerActivated:(NSNotification *)notification {
    if (_controllerContainerView.superview) {
        [_controllerContainerView removeFromSuperview];
    }
}

- (void)controllerDeactivated:(NSNotification *)notification {
    if ([[GCController controllers] count] == 0) {
        [self.view addSubview:_controllerContainerView];
    }
}

- (void)controllerPressedDPad:(iNDSDirectionalControlDirection) state
{
    if (state != _previousDirection && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 1)
        {
            [self vibrateSoft];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 2) {
            [self vibrateStrong];
        }
    }
    EMU_setDPad(state & iNDSDirectionalControlDirectionUp, state & iNDSDirectionalControlDirectionDown, state & iNDSDirectionalControlDirectionLeft, state & iNDSDirectionalControlDirectionRight);
    
    _previousDirection = state;
}

- (void)controllerPressedABXY:(iNDSButtonControlButton) state
{
    if (state != _previousButtons && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 1)
        {
            [self vibrateSoft];
        } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 2) {
            [self vibrateStrong];
        }
    }
    
    EMU_setABXY(state & iNDSButtonControlButtonA, state & iNDSButtonControlButtonB, state & iNDSButtonControlButtonX, state & iNDSButtonControlButtonY);
    
    _previousButtons = state;
}

- (void)controllerOnButtonUp:(BUTTON_ID) button
{
    EMU_buttonUp(button);
}

- (void) controllerOnButtonDown:(BUTTON_ID) button
{
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 1)
    {
        [self vibrateSoft];
    } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 2) {
        [self vibrateStrong];
    }
    EMU_buttonDown(button);
}

#pragma mark - Controls Delegate

- (IBAction)pressedDPad:(iNDSDirectionalControl *)sender
{
    iNDSDirectionalControlDirection state = sender.direction;
    [self controllerPressedDPad:state];
}

- (IBAction)pressedABXY:(iNDSButtonControl *)sender
{
    iNDSButtonControlButton state = sender.selectedButtons;
    [self controllerPressedABXY:state];
}

- (IBAction)onButtonUp:(UIControl*)sender
{
    EMU_buttonUp((BUTTON_ID)sender.tag);
}

- (IBAction)onButtonDown:(UIControl*)sender
{
    if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 1)
    {
        [self vibrateSoft];
    } else if ([[NSUserDefaults standardUserDefaults] integerForKey:@"vibrationStr"] == 2) {
        [self vibrateStrong];
    }
    EMU_buttonDown((BUTTON_ID)sender.tag);
}

FOUNDATION_EXTERN void AudioServicesStopSystemSound(int);
FOUNDATION_EXTERN void AudioServicesPlaySystemSoundWithVibration(unsigned long, objc_object*, NSDictionary*);

- (void)vibrateSoft
{
    //NSLog(@"Soft.");
    if (settingsShown || inEditingMode) return;
    // If force touch is avaliable we can assume taptic vibration is too
    if ([[self.view traitCollection] respondsToSelector:@selector(forceTouchCapability)] && ([[self.view traitCollection] forceTouchCapability] == UIForceTouchCapabilityAvailable) && [[NSUserDefaults standardUserDefaults] boolForKey:@"hapticForVibration"]) {
        [(UISelectionFeedbackGenerator*) vibration selectionChanged];
    } else {
        AudioServicesStopSystemSound(kSystemSoundID_Vibrate);
        
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        NSArray *pattern = @[@YES, @20, @NO, @1];
        
        dictionary[@"VibePattern"] = pattern;
        dictionary[@"Intensity"] = @1;
        
        AudioServicesPlaySystemSoundWithVibration(kSystemSoundID_Vibrate, nil, dictionary);
    }
}

- (void)vibrateStrong
{
    //NSLog(@"Hard");
    if (settingsShown || inEditingMode) return;// If force touch is avaliable we can assume taptic vibration is too
    if ([[self.view traitCollection] respondsToSelector:@selector(forceTouchCapability)] && ([[self.view traitCollection] forceTouchCapability] == UIForceTouchCapabilityAvailable) && [[NSUserDefaults standardUserDefaults] boolForKey:@"hapticForVibration"]) {
        [(UIImpactFeedbackGenerator*) vibration impactOccurred];
    } else {
        AudioServicesStopSystemSound(kSystemSoundID_Vibrate);
        
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        NSArray *pattern = @[@NO, @0, @YES, @30];
        
        dictionary[@"VibePattern"] = pattern;
        dictionary[@"Intensity"] = @1;
        
        AudioServicesPlaySystemSoundWithVibration(kSystemSoundID_Vibrate, nil, dictionary);
    }
}

- (void)touchScreenAtPoint:(CGPoint)point
{
    if (!disableTouchScreen) {
        point = CGPointApplyAffineTransform(point, CGAffineTransformMakeScale(256/glkView[1].bounds.size.width, 192/glkView[1].bounds.size.height));
        EMU_touchScreenTouch(point.x, point.y);
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (settingsShown && !inEditingMode) {
        [self toggleSettings:self];
        return;
    } else if (inEditingMode) { // gesture recognizers don't work on glkviews so we need to do it manually
        CGPoint location = [touches.anyObject locationInView:self.view];
        if (CGRectContainsPoint(glkView[0].frame, location) && !extWindow) {
            [self.profile handlePan:glkView[0] Location:location state:UIGestureRecognizerStateBegan];
            movingView = glkView[0];
        } else if (CGRectContainsPoint(glkView[1].frame, location)) {
            [self.profile handlePan:glkView[1] Location:location state:UIGestureRecognizerStateBegan];
            movingView = glkView[1];
        } else {
            [self.profile deselectView];
        }
    } else {
        for (UITouch *t in touches) {
            [self touchScreenAtPoint:[t locationInView:glkView[1]]];
        }
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (inEditingMode) { // promatically adding gesture recognizers to glkviews doesn't work
        CGPoint location = [touches.anyObject locationInView:self.view];
        if (CGRectContainsPoint(glkView[0].frame, location) && movingView == glkView[0] && !extWindow) {
            [self.profile handlePan:glkView[0] Location:location state:UIGestureRecognizerStateChanged];
        } else if (CGRectContainsPoint(glkView[1].frame, location) && movingView == glkView[1]) {
            [self.profile handlePan:glkView[1] Location:location state:UIGestureRecognizerStateChanged];
        }
    } else {
        for (UITouch *t in touches) {
            [self touchScreenAtPoint:[t locationInView:glkView[1]]];
        }
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (inEditingMode) { //esture recognizers don't work on glkviews so we need to do it manually
        CGPoint location = [touches.anyObject locationInView:self.view];
        if (CGRectContainsPoint(glkView[0].frame, location) && movingView == glkView[0]) {
            [self.profile handlePan:glkView[0] Location:location state:UIGestureRecognizerStateEnded];
        } else if (CGRectContainsPoint(glkView[1].frame, location) && movingView == glkView[1]) {
            [self.profile handlePan:glkView[1] Location:location state:UIGestureRecognizerStateEnded];
        }
        movingView = nil;
    } else {
        EMU_touchScreenRelease();
    }
}

#pragma mark - iCade Mode

- (void) setICadeMapping {
    //    TODO: Allow users to change this
    NSDictionary *_8BitdoNES30ProMapping = @{
                              ICADE_BTN_NUMBER(iCadeJoystickUp):    @{@"buttonDown": ^{[self controllerPressedDPad:iNDSDirectionalControlDirectionUp];},
                                                                      @"buttonUp"  : ^{[self controllerPressedDPad:0];}},
                              
                              ICADE_BTN_NUMBER(iCadeJoystickRight): @{@"buttonDown": ^{[self controllerPressedDPad:iNDSDirectionalControlDirectionRight];},
                                                                      @"buttonUp"  : ^{[self controllerPressedDPad:0];}},
                                             
                              ICADE_BTN_NUMBER(iCadeJoystickDown):  @{@"buttonDown": ^{[self controllerPressedDPad:iNDSDirectionalControlDirectionDown];},
                                                                      @"buttonUp"  : ^{[self controllerPressedDPad:0];}},
                                             
                              ICADE_BTN_NUMBER(iCadeJoystickLeft):  @{@"buttonDown": ^{[self controllerPressedDPad:iNDSDirectionalControlDirectionLeft];},
                                                                      @"buttonUp"  : ^{[self controllerPressedDPad:0];}},

                              ICADE_BTN_NUMBER(iCadeButtonA):       @{@"buttonDown": ^{[self controllerPressedABXY:iNDSButtonControlButtonX];},
                                                                      @"buttonUp"  : ^{[self controllerPressedABXY:0];}},

                              ICADE_BTN_NUMBER(iCadeButtonB):       @{@"buttonDown": ^{[self controllerPressedABXY:iNDSButtonControlButtonB];},
                                                                      @"buttonUp"  : ^{[self controllerPressedABXY:0];}},

                              ICADE_BTN_NUMBER(iCadeButtonC):       @{@"buttonDown": ^{[self controllerPressedABXY:iNDSButtonControlButtonA];},
                                                                      @"buttonUp"  : ^{[self controllerPressedABXY:0];}},

                              ICADE_BTN_NUMBER(iCadeButtonD):       @{@"buttonDown": ^{[self controllerPressedABXY:iNDSButtonControlButtonY];},
                                                                      @"buttonUp"  : ^{[self controllerPressedABXY:0];}},

                              ICADE_BTN_NUMBER(iCadeButtonE):       @{@"buttonDown": ^{[self controllerOnButtonDown:BUTTON_R];},
                                                                      @"buttonUp"  : ^{[self controllerOnButtonUp:BUTTON_R];}},

                              ICADE_BTN_NUMBER(iCadeButtonF):       @{@"buttonDown": ^{[self controllerOnButtonDown:BUTTON_L];},
                                                                      @"buttonUp"  : ^{[self controllerOnButtonUp:BUTTON_L];}},

                              ICADE_BTN_NUMBER(iCadeButtonG):       @{@"buttonDown": ^{[self controllerOnButtonDown:BUTTON_START];},
                                                                      @"buttonUp"  : ^{[self controllerOnButtonUp:BUTTON_START];}},

                              ICADE_BTN_NUMBER(iCadeButtonH):       @{@"buttonDown": ^{[self controllerOnButtonDown:BUTTON_SELECT];},
                                                                      @"buttonUp"  : ^{[self controllerOnButtonUp:BUTTON_SELECT];}},
                                             };
    
    iCadeMapping = _8BitdoNES30ProMapping;
}

- (void)setState:(BOOL)state forButton:(iCadeState)button {
    NSString *buttonState = state ? @"buttonDown" : @"buttonUp";
    
    void (^keyCallback)(void) = iCadeMapping [ICADE_BTN_NUMBER(button)] [buttonState];
    keyCallback();
}

#pragma mark - iCadeDelegate
- (void)buttonDown:(iCadeState)button {
    [self setState:YES forButton:button];
}

- (void)buttonUp:(iCadeState)button {
    [self setState:NO forButton:button];
}
@end


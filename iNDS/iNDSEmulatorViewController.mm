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
    
    dispatch_semaphore_t displaySemaphore;
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
    
    disableTouchScreen = [[NSUserDefaults standardUserDefaults] boolForKey:@"disableTouchScreen"];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [UIApplication sharedApplication].statusBarHidden = YES;
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self pauseEmulation];
    [UIApplication sharedApplication].statusBarHidden = NO;
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

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
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
        EMU_enableSound(![defaults boolForKey:@"disableSound"]);
        EMU_setSynchMode([defaults boolForKey:@"synchSound"]);
        // Enable sound?
        // (Mute on && don't ignore it) or user has sound disabled
        BOOL muteSound = ([self muteButtonOn] && ![defaults boolForKey:@"ignoreMute"]) || [defaults boolForKey:@"disableSound"];
        EMU_enableSound(!muteSound);
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord
                                         withOptions:(AVAudioSessionCategoryOptionAllowBluetooth |
                                                      AVAudioSessionCategoryOptionMixWithOthers |
                                                      AVAudioSessionCategoryOptionDefaultToSpeaker)
                                               error:nil];
        
        // Filter
        int filterTranslate[] = {NONE, EPX, SUPEREAGLE, _2XSAI, SUPER2XSAI, BRZ2x, LQ2X, BRZ3x, HQ2X, HQ4X, BRZ4x, BRZ5x};
        NSInteger filter = [[NSUserDefaults standardUserDefaults] integerForKey:@"videoFilter"];
        EMU_setFilter(filterTranslate[filter]);
    }
    self.directionalControl.style = [defaults integerForKey:@"controlPadStyle"];
    self.fpsLabel.hidden = ![defaults integerForKey:@"showFPS"];
    
    // For some reason both of these disable the mic
    if ([defaults integerForKey:@"volumeBumper"] && !settingsShown) {
        [volumeStealer startStealingVolumeButtonEvents];
    } else {
        [volumeStealer stopStealingVolumeButtonEvents];
    }
    
    [self.view setNeedsLayout];
}

// Not sure if we can figure this out
- (BOOL)muteButtonOn
{
    return NO;
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
    EMU_setCPUMode(1);//[[NSUserDefaults standardUserDefaults] boolForKey:@"enableLightningJIT"] ? 2 : 1);
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
    
    //ToDo: make this better
    BOOL bicubic = [[NSUserDefaults standardUserDefaults] boolForKey:@"highresGraphics"];
    if (bicubic) {
        kFragShader = [NSString stringWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"] encoding:NSUTF8StringEncoding error:nil];
    } else {
        kFragShader = SHADER_STRING (
                       uniform sampler2D inputImageTexture;
                       varying highp vec2 texCoord;
                       
                       void main()
                       {
                           highp vec4 color = texture2D(inputImageTexture, texCoord);
                           gl_FragColor = color;
                       }
                       );
    }
    
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bicubic ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    //Window 2
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bicubic ? GL_NEAREST : GL_LINEAR);
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
    [self.view endEditing:YES];
    [self updateDisplay]; //This has to be called once before we touch or move any glk views
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        displaySemaphore = dispatch_semaphore_create(1);
        CGFloat framesToRender = 0;
        lastAutosave = CACurrentMediaTime();
        [emuLoopLock lock];
        [[iNDSMFIControllerSupport instance] startMonitoringGamePad];
        CFTimeInterval coreStart, loopStart;
        coreFps = videoFps = 30;
        while (execute) {
            loopStart = CACurrentMediaTime();
            framesToRender += self.speed;
            
            for (;framesToRender >= 1; framesToRender--) {
                coreStart = CACurrentMediaTime();
                EMU_runCore();
                coreFps = coreFps * 0.95 + (1 / (CACurrentMediaTime() - coreStart)) * 0.05;
            }
            
            if (CACurrentMediaTime() - lastAutosave > 180) {
                CGFloat coreTime = [[NSUserDefaults standardUserDefaults] floatForKey:@"coreTime"];
                coreTime = coreTime * 0.95 + (CACurrentMediaTime() - coreStart) * 0.05;
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"periodicSave"]) {
                    [self saveStateWithName:[NSString stringWithFormat:@"Auto Save"]];
                }
                lastAutosave = CACurrentMediaTime();
            }
            
            // Core will always be one frame ahead
            dispatch_semaphore_wait(displaySemaphore, DISPATCH_TIME_FOREVER);
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                EMU_copyMasterBuffer();
                //This will automatically throttle fps to 60
                [self updateDisplay];
                dispatch_semaphore_signal(displaySemaphore);
            }); 
        }
        [[iNDSMFIControllerSupport instance] stopMonitoringGamePad];
        [emuLoopLock unlock];
    });
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
    if (texHandle[0] == 0) return;
    static CFTimeInterval lastDisplayTime = 0;
    videoFps = videoFps * 0.95 + (1 / (CACurrentMediaTime() - lastDisplayTime)) * 0.05;
    dispatch_async(dispatch_get_main_queue(), ^{
        static CFTimeInterval fpsUpdateTime = 0;
        if (CACurrentMediaTime() - fpsUpdateTime > 1) {
            self.fpsLabel.text = [NSString stringWithFormat:@"%d FPS %d Max Core", MIN((int)videoFps, 60), (int)(coreFps / self.speed)];
            fpsUpdateTime = CACurrentMediaTime();
        }
    });
    
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
    lastDisplayTime = CACurrentMediaTime();
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


#pragma mark - Controls

- (void) setSpeed:(CGFloat)speed
{
    if (!self.program) return;
    int userFrameSkip = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"frameSkip"];
    if (speed < 1) { //Force this. Audio sounds so bad < 1 when synched
        EMU_setSynchMode(0);
    } else {
        EMU_setSynchMode([[NSUserDefaults standardUserDefaults] boolForKey:@"synchSound"]);
    }
    _speed = speed;
}

- (void)setLidClosed:(BOOL)closed
{
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

- (IBAction)pressedDPad:(iNDSDirectionalControl *)sender
{
    iNDSDirectionalControlDirection state = sender.direction;
    
    EMU_setDPad(state & iNDSDirectionalControlDirectionUp, state & iNDSDirectionalControlDirectionDown, state & iNDSDirectionalControlDirectionLeft, state & iNDSDirectionalControlDirectionRight);
    
    _previousDirection = state;
}

- (IBAction)pressedABXY:(iNDSButtonControl *)sender
{
    iNDSButtonControlButton state = sender.selectedButtons;
    if (state != _previousButtons && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
        {
            [self vibrate];
        }
    }
    
    EMU_setABXY(state & iNDSButtonControlButtonA, state & iNDSButtonControlButtonB, state & iNDSButtonControlButtonX, state & iNDSButtonControlButtonY);
    
    _previousButtons = state;
}

- (IBAction)onButtonUp:(UIControl*)sender
{
    EMU_buttonUp((BUTTON_ID)sender.tag);
}

- (IBAction)onButtonDown:(UIControl*)sender
{
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
    {
        [self vibrate];
    }
    EMU_buttonDown((BUTTON_ID)sender.tag);
}

FOUNDATION_EXTERN void AudioServicesStopSystemSound(int);
FOUNDATION_EXTERN void AudioServicesPlaySystemSoundWithVibration(unsigned long, objc_object*, NSDictionary*);

- (void)vibrate
{
    if (settingsShown || inEditingMode) return;
    // If force touch is avaliable we can assume taptic vibration is too
    if ([[self.view traitCollection] respondsToSelector:@selector(forceTouchCapability)] && [[self.view traitCollection] forceTouchCapability] == UIForceTouchCapabilityAvailable) {
        [[[UIDevice currentDevice] tapticEngine] actuateFeedback:UITapticEngineFeedbackPeek];
    } else {
        AudioServicesStopSystemSound(kSystemSoundID_Vibrate);
        
        NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
        NSArray *pattern = @[@YES, @20, @NO, @1];
        
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
    UIView * statusBar = [self statuBarView];
    if (!settingsShown) { //About to show settings
        [volumeStealer performSelector:@selector(stopStealingVolumeButtonEvents) withObject:nil afterDelay:0.1]; //Non blocking
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
           [[UIApplication sharedApplication] setStatusBarHidden:NO];
            statusBar.alpha = 0;
        }
        [self.settingsContainer setHidden:NO];
        [self pauseEmulation];
        [UIView animateWithDuration:0.3 animations:^{
            self.darkenView.hidden = NO;
            self.darkenView.alpha = 0.6;
            self.settingsContainer.alpha = 1;
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"fullScreenSettings"]) {
                statusBar.alpha = 1;
            }
        } completion:^(BOOL finished) {
            settingsShown = YES;
            if (!inEditingMode)
                [CHBgDropboxSync start];
        }];
    } else {
        if ([[NSUserDefaults standardUserDefaults] integerForKey:@"volumeBumper"]) {
            [volumeStealer performSelector:@selector(startStealingVolumeButtonEvents) withObject:nil afterDelay:0.1];
        }
        [UIView animateWithDuration:0.3 animations:^{
            self.darkenView.alpha = 0.0;
            self.settingsContainer.alpha = 0;
            statusBar.alpha = 0;
        } completion:^(BOOL finished) {
            [[UIApplication sharedApplication] setStatusBarHidden:YES];
            settingsShown = NO;
            self.settingsContainer.hidden = YES;
            [self resumeEmulation];
            self.darkenView.hidden = YES;
        }];

        disableTouchScreen = [[NSUserDefaults standardUserDefaults] boolForKey:@"disableTouchScreen"];
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
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([segue.identifier isEqualToString:@"SettingsEmbed"]) {
        settingsNav = segue.destinationViewController;
    }
}


#pragma mark - iCade Mode
- (void)setState:(BOOL)state forButton:(iCadeState)button {
    if (state) {
        switch (button) {
            case iCadeButtonA:
                [self oniCadeButtonDown:BUTTON_SELECT];
                break;
            case iCadeButtonB:
                [self oniCadeButtonDown:BUTTON_L];
                break;
            case iCadeButtonC:
                [self oniCadeButtonDown:BUTTON_START];
                break;
            case iCadeButtonD:
                [self oniCadeButtonDown:BUTTON_R];
                break;
            case iCadeButtonE:
                [self pressediCadeABXY:iNDSButtonControlButtonY];
                break;
            case iCadeButtonF:
                [self pressediCadeABXY:iNDSButtonControlButtonX];
                break;
            case iCadeButtonG:
                [self pressediCadeABXY:iNDSButtonControlButtonB];
                break;
            case iCadeButtonH:
                [self pressediCadeABXY:iNDSButtonControlButtonA];
                break;
                
            case iCadeJoystickUp:
                [self pressediCadePad:iNDSDirectionalControlDirectionUp];
                break;
            case iCadeJoystickRight:
                [self pressediCadePad:iNDSDirectionalControlDirectionRight];
                break;
            case iCadeJoystickDown:
                [self pressediCadePad:iNDSDirectionalControlDirectionDown];
                break;
            case iCadeJoystickLeft:
                [self pressediCadePad:iNDSDirectionalControlDirectionLeft];
                break;
            default:
                break;
        }
    }
    else {
        switch (button) {
            case iCadeButtonE:
                [self pressediCadeABXY:0];
                break;
            case iCadeButtonF:
                [self pressediCadeABXY:0];
                break;
            case iCadeButtonG:
                [self pressediCadeABXY:0];
                break;
            case iCadeButtonH:
                [self pressediCadeABXY:0];
                break;

            case iCadeJoystickUp:
                [self pressediCadePad:0];
                break;
            case iCadeJoystickRight:
                [self pressediCadePad:0];
                break;
            case iCadeJoystickDown:
                [self pressediCadePad:0];
                break;
            case iCadeJoystickLeft:
                [self pressediCadePad:0];
                break;
            default:
                break;
        }
    }
}

- (void)pressediCadePad:(iNDSDirectionalControlDirection)state {
    if (state != _previousDirection && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
        {
            [self vibrate];
        }
    }
    
    EMU_setDPad(state & iNDSDirectionalControlDirectionUp, state & iNDSDirectionalControlDirectionDown, state & iNDSDirectionalControlDirectionLeft, state & iNDSDirectionalControlDirectionRight);
    
    _previousDirection = state;
}

- (void)pressediCadeABXY:(iNDSButtonControlButton)state {
    if (state != _previousButtons && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"])
        {
            [self vibrate];
        }
    }
    
    EMU_setABXY(state & iNDSButtonControlButtonA, state & iNDSButtonControlButtonB, state & iNDSButtonControlButtonX, state & iNDSButtonControlButtonY);
    
    _previousButtons = state;
}

- (void)oniCadeButtonUp:(BUTTON_ID)buttonId {
    EMU_buttonUp(buttonId);
}

- (void)oniCadeButtonDown:(BUTTON_ID)buttonId {
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"]) {
        [self vibrate];
    }
    EMU_buttonDown(buttonId);
}

#pragma mark - iCadeDelegate
- (void)buttonDown:(iCadeState)button {
    [self setState:YES forButton:button];
}

- (void)buttonUp:(iCadeState)button {
    [self setState:NO forButton:button];
}
@end


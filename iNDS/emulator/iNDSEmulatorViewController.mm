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

#import <GLKit/GLKit.h>
#import <OpenGLES/ES2/gl.h>
#import <AudioToolbox/AudioToolbox.h>
#import <GameController/GameController.h>

#include "emu.h"

#import "iNDSMFIControllerSupport.h"
#import "iNDSEmulationProfile.h"

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

NSString *const kFragShader = SHADER_STRING
(
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

@interface iNDSEmulatorViewController () <GLKViewDelegate> {
    int fps;
    
    GLuint texHandle[2];
    GLint attribPos;
    GLint attribTexCoord;
    GLint texUniform;
    
    GLKView *glkView[2];
    
    iNDSButtonControlButton _previousButtons;
    iNDSDirectionalControlDirection _previousDirection;
    
    NSLock *emuLoopLock;
    
    UIWindow *extWindow;
    
    CFTimeInterval lastAutosave;
    
    NSInteger speed;
    
    iNDSEmulationProfile * profile;
}

@property (weak, nonatomic) IBOutlet UILabel *fpsLabel;
@property (weak, nonatomic) IBOutlet UIImageView *pixelGrid;
@property (strong, nonatomic) GLProgram *program;
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) IBOutlet UIView *controllerContainerView;

@property (weak, nonatomic) IBOutlet iNDSDirectionalControl *directionalControl;
@property (weak, nonatomic) IBOutlet iNDSButtonControl *buttonControl;
@property (weak, nonatomic) IBOutlet UIButton *dismissButton;
@property (weak, nonatomic) IBOutlet UIButton *startButton;
@property (weak, nonatomic) IBOutlet UIButton *cheatsButton;
@property (weak, nonatomic) IBOutlet UIButton *selectButton;
@property (weak, nonatomic) IBOutlet UIButton *speedButton;
@property (weak, nonatomic) IBOutlet UIButton *leftTrigger;
@property (weak, nonatomic) IBOutlet UIButton *rightTrigger;
@property (strong, nonatomic) UIImageView *snapshotView;

- (IBAction)hideEmulator:(id)sender;
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
	// Do any additional setup after loading the view.
    
    profile = [[iNDSEmulationProfile alloc] init];
    
    self.view.multipleTouchEnabled = YES;
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter addObserver:self selector:@selector(pauseEmulation) name:UIApplicationWillResignActiveNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(resumeEmulation) name:UIApplicationDidBecomeActiveNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(defaultsChanged:) name:NSUserDefaultsDidChangeNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(screenChanged:) name:UIScreenDidConnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(screenChanged:) name:UIScreenDidDisconnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(controllerActivated:) name:GCControllerDidConnectNotification object:nil];
    [notificationCenter addObserver:self selector:@selector(controllerDeactivated:) name:GCControllerDidDisconnectNotification object:nil];
    
    if ([[GCController controllers] count] > 0) {
        [self controllerActivated:nil];
    }
    [self defaultsChanged:nil];
    
    speed = 1;
    
    self.cheatsButton.layer.cornerRadius = self.speedButton.layer.cornerRadius = 2;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [UIApplication sharedApplication].statusBarHidden = YES;
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self pauseEmulation];
    [self saveStateWithName:nil];
    [UIApplication sharedApplication].statusBarHidden = NO;
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self loadROM];
    [self defaultsChanged:nil];
    
    //NSString * testCheat = @"9208B3F4FFFD0002920CA88000000000021B6ABO00004000D2000000000000009208B3F4FFFE0001920CA88000000000021B6AB000004000D200000000000000";
    //NSLog(@"Adding Cheat: %@", testCheat);
    //EMU_add_AR("9208B3F4FFFD0002920CA88000000000021B6ABO00004000D2000000000000009208B3F4FFFE0001920CA88000000000021B6AB000004000D200000000000000", "Big Jump", YES);
    //EMU_add_AR("03807D40EBAFD8AF", "Master Code", true);
    //EMU_add_AR("221B6ACD00000000221B6AD100000000221B6AD500000000", "Tiny Mario", YES);
    //EMU_add_AR("1208b3340000027c2208b32400000003", "Giant Mario", true);
    /*EMU_addCheat(3, 0x620c2b54, 0x00000000, "Master Code", true);
    EMU_addCheat(3, 0xb20c2b54, 0x00000000, "A", true);
    EMU_addCheat(3, 0x1000015c, 0x00000001, "B", true);
    EMU_addCheat(3, 0x10000160, 0x000003e7, "C", true);
    EMU_addCheat(3, 0xd2000000, 0x00000000, "D", true);*/
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

- (void)defaultsChanged:(NSNotification*)notification
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (emuLoopLock) { //Only update these is the core has loaded
        EMU_setFrameSkip((int)[defaults integerForKey:@"frameSkip"]);
        EMU_enableSound(![defaults boolForKey:@"disableSound"]);
        EMU_setSynchMode([defaults boolForKey:@"synchSound"]);
    }
    self.directionalControl.style = [defaults integerForKey:@"controlPadStyle"];
    
    [self viewWillLayoutSubviews];
    
    // Purposefully commented out line below, as we don't want to be able to switch CPU modes in the middle of emulation
    // EMU_setCPUMode([defaults boolForKey:@"enableLightningJIT"] ? 2 : 1);
    
    
    self.fpsLabel.hidden = ![defaults integerForKey:@"showFPS"];
    self.pixelGrid.hidden = YES;//![defaults integerForKey:@"showPixelGrid"];
}

- (void)viewWillLayoutSubviews
{
    
}


- (void)dealloc
{
    EMU_closeRom();
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)screenChanged:(NSNotification*)notification
{
    [self pauseEmulation];
    [self performSelector:@selector(resumeEmulation) withObject:nil afterDelay:0.5];
}

#pragma mark - Playing ROM

- (void)loadROM {
    EMU_setWorkingDir([[self.game.path stringByDeletingLastPathComponent] fileSystemRepresentation]);
    EMU_init([iNDSGame preferredLanguage]);
    EMU_setCPUMode(1);//[[NSUserDefaults standardUserDefaults] boolForKey:@"enableLightningJIT"] ? 2 : 1);
    EMU_loadRom([self.game.path fileSystemRepresentation]);
    EMU_change3D(1);
        
    [self initGL];
    
    emuLoopLock = [NSLock new];
    
    if (self.saveState) EMU_loadState(self.saveState.fileSystemRepresentation);
    [self startEmulatorLoop];
}

- (void)initGL
{
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:self.context];
    
    if ([UIScreen screens].count > 1) {
        UIScreen *extScreen = [UIScreen screens][1];
        extScreen.currentMode = extScreen.availableModes[0];
        extWindow = [[UIWindow alloc] initWithFrame:extScreen.bounds];
        extWindow.screen = extScreen;
        extWindow.backgroundColor = [UIColor orangeColor];
        glkView[0] = [[GLKView alloc] initWithFrame:extWindow.bounds context:self.context];
        glkView[1] = [[GLKView alloc] initWithFrame:profile.touchScreenRect context:self.context];
        glkView[0].delegate = self;
        glkView[1].delegate = self;
        [self.view insertSubview:glkView[1] atIndex:0];
        [extWindow addSubview:glkView[0]];
        [extWindow makeKeyAndVisible];
    } else {
        UIScreen *extScreen = [UIScreen screens][1];
        extScreen.currentMode = extScreen.availableModes[0];
        extWindow = [[UIWindow alloc] initWithFrame:extScreen.bounds];
        extWindow.screen = extScreen;
        extWindow.backgroundColor = [UIColor orangeColor];
        glkView[0] = [[GLKView alloc] initWithFrame:profile.mainScreenRect context:self.context];
        glkView[1] = [[GLKView alloc] initWithFrame:profile.touchScreenRect context:self.context];
        glkView[0].delegate = self;
        glkView[1].delegate = self;
        [self.view insertSubview:glkView[1] atIndex:0];
        [extWindow addSubview:glkView[0]];
        [extWindow makeKeyAndVisible];
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
    
    float scale = [UIScreen mainScreen].scale;
    CGSize size = CGSizeMake(glkView[1].bounds.size.width * scale, glkView[1].bounds.size.height * scale);
    
    glViewport(0, 0, size.width, size.height);
    
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
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    size_t dataSize = 0;
    UInt8 *dataBytes = (UInt8*)EMU_getVideoBuffer(&dataSize);
    if (num >= 0) dataSize /= 2;
    if (num == 1) dataBytes += dataSize;
    CFDataRef videoData = CFDataCreate(NULL, dataBytes, dataSize*4);
    CGDataProviderRef dataProvider = CGDataProviderCreateWithCFData(videoData);
    CGImageRef screenImage = CGImageCreate(256, num < 0 ? 384 : 192, 8, 32, 256*4, colorSpace, kCGBitmapByteOrderDefault, dataProvider, NULL, false, kCGRenderingIntentDefault);
    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(dataProvider);
    CFRelease(videoData);
    
    UIImage *image = [UIImage imageWithCGImage:screenImage];
    CGImageRelease(screenImage);
    return image;
}

- (void)pauseEmulation
{
    if (!execute) return;
    // save snapshot of screen
    /*if (self.snapshotView == nil) {
        self.snapshotView = [[UIImageView alloc] initWithFrame:glkView[extWindow?1:0].frame];
        [self.view insertSubview:self.snapshotView aboveSubview:glkView[extWindow?1:0]];
    } else {
        self.snapshotView.hidden = NO;
    }
    self.snapshotView.image = [self screenSnapshot:extWindow?1:-1];
    NSLog(@"%@", self.snapshotView.image);*/
    // pause emulation
    EMU_pause(true);
    [emuLoopLock lock]; // make sure emulator loop has ended
    [emuLoopLock unlock];
    [self shutdownGL];
}

- (void)resumeEmulation
{
    if (self.presentingViewController.presentedViewController != self) return;
    if (execute) return;
    // remove snapshot
    self.snapshotView.hidden = YES;
    
    // resume emulation
    [self initGL];
    EMU_pause(false);
    [self startEmulatorLoop];
}

- (void)startEmulatorLoop
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        lastAutosave = CACurrentMediaTime();
        [emuLoopLock lock];
        [[iNDSMFIControllerSupport instance] startMonitoringGamePad];
        while (execute) {
            for (int i = 0; i < speed; i++) {
                //Enabling this can provide full emulation speed on most devices but will reduce fps
                
                //for (int j = 0; j < MIN(MAX(60 / fps, 6), 1); j++) { //10 - 60 fps
                    EMU_runCore();
                //}
            }
            fps = EMU_runOther();
            EMU_copyMasterBuffer();
            [self updateDisplay];
            if (CACurrentMediaTime() - lastAutosave > 180) {
                NSString *lastAutosavePath = [self.game pathForSaveStateWithName:@"Auto Save"];
                if ([[NSFileManager defaultManager] fileExistsAtPath:lastAutosavePath]) {
                    [[NSFileManager defaultManager] removeItemAtPath:lastAutosavePath error:nil];
                }
                [self saveStateWithName:[NSString stringWithFormat:@"Auto Save"]];
                lastAutosave = CACurrentMediaTime();
            }
        }
        
        [[iNDSMFIControllerSupport instance] stopMonitoringGamePad];
        [emuLoopLock unlock];
    });
}

- (void)saveStateWithName:(NSString*)saveStateName
{
    if (self.saveState == nil || saveStateName != nil) self.saveState = [self.game pathForSaveStateWithName:saveStateName ? saveStateName : @"Pause"];
    EMU_saveState(self.saveState.fileSystemRepresentation);
    [self.game reloadSaveStates];
}

- (void)updateDisplay
{
    if (texHandle[0] == 0) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        self.fpsLabel.text = [NSString stringWithFormat:@"%d FPS",fps];
    });
    
    GLubyte *screenBuffer = (GLubyte*)EMU_getVideoBuffer(NULL);
    glBindTexture(GL_TEXTURE_2D, texHandle[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);
    [glkView[0] display];
    
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer + 256*192*4);
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

#pragma mark - Cheats

- (IBAction)showCheats:(id)sender
{
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Cheats not implemented"
                                                message:@"Cheats have not been implemented yet but they'll be here soon!"
                                               delegate:nil 
                                      cancelButtonTitle:@"OK"
                                      otherButtonTitles:nil];
    [alert show];
}

#pragma mark - Controls

- (IBAction)changeSpeed:(id)sender
{
    switch (speed) {
      case 1:
        speed = 2;
        [self.speedButton setTitle:@"2x" forState:UIControlStateNormal];
        EMU_setFrameSkip(2);
        self.speedButton.alpha = 1;
        break;
      
      case 2:
        speed = 4;
        EMU_setFrameSkip(4);
        [self.speedButton setTitle:@"4x" forState:UIControlStateNormal];
        self.speedButton.alpha = 1;
        break;
    
      case 4:
        speed = 1;
        [self.speedButton setTitle:@"1x" forState:UIControlStateNormal];
        self.speedButton.alpha = MAX(MAX(0.1, [[NSUserDefaults standardUserDefaults] floatForKey:@"controlOpacity"]), self.view.bounds.size.width > self.view.bounds.size.height);
        EMU_setFrameSkip((int)[[NSUserDefaults standardUserDefaults] integerForKey:@"frameSkip"]);
        break;
    }
}

- (void)controllerActivated:(NSNotification *)notification {
    if (_controllerContainerView.superview) {
        [_controllerContainerView removeFromSuperview];
    }
}

- (void)controllerDeactivated:(NSNotification *)notification {
    if ([[GCController controllers] count] == 0) {
        CGRect controllerContainerFrame = _controllerContainerView.frame;
        controllerContainerFrame.origin.x = 0;
        controllerContainerFrame.origin.y = self.view.frame.size.height-controllerContainerFrame.size.height;
        controllerContainerFrame.size.width = self.view.frame.size.width;
        _controllerContainerView.frame = controllerContainerFrame;
        [self.view addSubview:_controllerContainerView];
    }
}

- (IBAction)pressedDPad:(iNDSDirectionalControl *)sender
{
    iNDSDirectionalControlDirection state = sender.direction;
    
    if (state != _previousDirection && state != 0)
    {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"vibrate"] && ![[NSUserDefaults standardUserDefaults] boolForKey: @"controlPadStyle"])
        {
            [self vibrate];
        }
    }
    
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
    AudioServicesStopSystemSound(kSystemSoundID_Vibrate);
    
    NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
    NSArray *pattern = @[@YES, @20, @NO, @1];
    
    dictionary[@"VibePattern"] = pattern;
    dictionary[@"Intensity"] = @1;
    
    AudioServicesPlaySystemSoundWithVibration(kSystemSoundID_Vibrate, nil, dictionary);
}

- (void)touchScreenAtPoint:(CGPoint)point
{
    if (glkView[1] != nil) {
        // glkView[1] is touch screen
        point = CGPointApplyAffineTransform(point, CGAffineTransformMakeScale(256/glkView[1].bounds.size.width, 192/glkView[1].bounds.size.height));
    } else {
        // bottom half of glkView[0] is touch screen
        if (point.y < glkView[0].bounds.size.height/2) return;
        CGAffineTransform t = CGAffineTransformConcat(CGAffineTransformMakeTranslation(0, -glkView[0].bounds.size.height/2), CGAffineTransformMakeScale(256/glkView[0].bounds.size.width, 192/(glkView[0].bounds.size.height/2)));
        point = CGPointApplyAffineTransform(point, t);
    }
    EMU_touchScreenTouch(point.x, point.y);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchScreenAtPoint:[[touches anyObject] locationInView:glkView[extWindow?1:0]]];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self touchScreenAtPoint:[[touches anyObject] locationInView:glkView[extWindow?1:0]]];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    EMU_touchScreenRelease();
}

- (IBAction)hideEmulator:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)doSaveState:(UILongPressGestureRecognizer*)sender
{
    if (![sender isKindOfClass:[UILongPressGestureRecognizer class]] || sender.state != UIGestureRecognizerStateBegan) return;
    UIAlertView *saveAlert = [[UIAlertView alloc] initWithTitle:@"Save State" message:@"Name for save state:" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Save", nil];
    saveAlert.alertViewStyle = UIAlertViewStylePlainTextInput;
    [saveAlert show];
}

#pragma mark Alert View Delegate

- (void)willPresentAlertView:(UIAlertView *)alertView
{
    [self pauseEmulation];
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) {
        [self resumeEmulation];
    }
    if (buttonIndex == 1) {
        // save
        NSString *saveStateName = [alertView textFieldAtIndex:0].text;
        [self saveStateWithName:saveStateName];
        [self dismissViewControllerAnimated:YES completion:nil];
    }
}

- (void)viewDidUnload {
    [super viewDidUnload];
}
@end

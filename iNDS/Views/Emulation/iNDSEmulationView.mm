//
//  iNDSEmulationView.m
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationView.h"

#import <GLKit/GLKit.h>
#import <OpenGLES/ES2/gl.h>
#import "GLProgram.h"
#import "main_iOS.h"

#include "GPU.h"
#include "gfx3d.h"

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


@interface iNDSEmulationView () <GLKViewDelegate> {
    float coreFps;
    float videoFps;
    
    GLuint texHandle[2];
    GLint attribPos;
    GLint attribTexCoord;
    GLint texUniform;
    
    u32* outputBuffer;
    
    GLKView *movingView;
}



@property GLKView *topView; // NDS main top screen
@property GLKView *bottomView; // NDS Touch screen
@property GLProgram *program;
@property EAGLContext *context;

@end

@implementation iNDSEmulationView

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        self.backgroundColor = [UIColor whiteColor];
        [self initGL];
    }
    return self;
}

- (void)initGL
{
    NSLog(@"Initiaizing GL");
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:self.context];
    
    self.topView = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
    self.bottomView = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
    self.topView.delegate = self;
    self.bottomView.delegate = self;
    [self insertSubview:self.bottomView atIndex:0];
    [self insertSubview:self.topView atIndex:0];
    
    self.topView.frame = CGRectMake(0, 0, 256, 192);
    self.bottomView.frame = CGRectMake(0, 200, 256, 192);
    
    
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
    
    outputBuffer = (u32 *)malloc([self bufferSize] * sizeof(uint32));
}

- (NSInteger)bufferSize
{
    return 256 * 192 * 2;
}

- (u8 *)videoBuffer
{
    return GPU_screen;
}

// Should offload this to GPU
- (void)RBG555:(u16 *)inBuffer TORBGA8888:(u32 *)outBuffer {
    int size = 256*192*2;
    for(int i=0;i<size;++i)
        *(outBuffer+i) = 0xFF000000 | RGB15TO32_NOALPHA(inBuffer[i]);
}

// Doesn't work right
- (void)RBG555:(u16 *)inBuffer TORBGA15:(u16 *)outBuffer {
    int size = 256*192*2;
    for(int i=0;i<size;++i) {
        *(outBuffer+i) = 0x8000 | inBuffer[i];
    }
}

- (void)updateDisplay
{
    if (texHandle[0] == 0 || !execute) return;
    
    NSLog(@"Updating Display2");
    size_t bufferSize = [self bufferSize];
    u16 *screenBuffer = (u16 *)[self videoBuffer];
    
    [self RBG555:screenBuffer TORBGA8888:outputBuffer];
    //[self RBG555:screenBuffer TORBGA15:outputBuffer];
    
    int scale = 1;//(int)sqrt(bufferSize/98304);//1, 3, 4
    
    int width = 256 * scale;
    int height = 192 * scale;
    
    glBindTexture(GL_TEXTURE_2D, texHandle[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outputBuffer);
    [self.topView display]; 
    
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, outputBuffer + width * height);
    [self.bottomView display];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (view == self.topView) ? texHandle[0] : texHandle[1]);
    glUniform1i(texUniform, 1);
    
    glVertexAttribPointer(attribPos, 2, GL_FLOAT, 0, 0, (const GLfloat*)&positionVert);
    glVertexAttribPointer(attribTexCoord, 2, GL_FLOAT, 0, 0, (const GLfloat*)&textureVert);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

@end

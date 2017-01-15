//
//  iNDSEmulationView.m
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import "iNDSEmulationView.h"

#import <OpenGLES/ES2/gl.h>
#import "GLProgram.h"
#import "emu.h"

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
}


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
    
    self.mainScreen = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
    self.touchScreen = [[GLKView alloc] initWithFrame:CGRectMake(0, 0, 4, 3) context:self.context];
    self.mainScreen.delegate = self;
    self.touchScreen.delegate = self;
    [self insertSubview:self.touchScreen atIndex:0];
    [self insertSubview:self.mainScreen atIndex:0];
    
    self.mainScreen.frame = CGRectMake(0, 0, 256, 192);
    self.touchScreen.frame = CGRectMake(0, 200, 256, 192);
    
    
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

- (NSInteger)bufferSize
{
    return 256 * 192 * 2;
}

- (void)updateDisplay
{
    if (texHandle[0] == 0 || !execute) return;
    u32 *screenBuffer = EMU_RBGA8Buffer();
    int scale = 1;//(int)sqrt(bufferSize/98304);//1, 3, 4
    
    int width = 256 * scale;
    int height = 192 * scale;
    
    glBindTexture(GL_TEXTURE_2D, texHandle[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);
    [self.mainScreen display];
    
    glBindTexture(GL_TEXTURE_2D, texHandle[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &screenBuffer[width * height]);
    [self.touchScreen display];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (view == self.mainScreen) ? texHandle[0] : texHandle[1]);
    glUniform1i(texUniform, 1);
    
    glVertexAttribPointer(attribPos, 2, GL_FLOAT, 0, 0, (const GLfloat*)&positionVert);
    glVertexAttribPointer(attribTexCoord, 2, GL_FLOAT, 0, 0, (const GLfloat*)&textureVert);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

#pragma mark Touch

- (void)touchScreenAtPoint:(CGPoint)point
{
    point = CGPointApplyAffineTransform(point, CGAffineTransformMakeScale(256/self.touchScreen.bounds.size.width, 192/self.touchScreen.bounds.size.height));
    EMU_touchScreenTouch(point.x, point.y);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches) {
        [self touchScreenAtPoint:[t locationInView:self.touchScreen]];
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    for (UITouch *t in touches) {
        [self touchScreenAtPoint:[t locationInView:self.touchScreen]];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    EMU_touchScreenRelease();
}

@end

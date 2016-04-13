//
//  iNDSMicrophone.h
//  iNDS
//
//  Created by Will Cobb on 4/6/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TPCircularBuffer.h"
@interface iNDSMicrophone : NSObject

@property TPCircularBuffer *buffer;

- (id)initWithBuffer:(TPCircularBuffer *)buffer;
- (void)start;
- (BOOL)micEnabled;

@end

//
//  UIScreen+Widescreen.m
//  iNDS
//
//  Created by iNDS on 6/2/13.
//  Copyright (c) 2013 Homebrew. All rights reserved.
//

#import "UIScreen+Widescreen.h"

@implementation UIScreen (Widescreen)

- (BOOL)isWidescreen {
    return [self bounds].size.height == 568;
}

@end

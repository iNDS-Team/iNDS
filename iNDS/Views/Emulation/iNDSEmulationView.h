//
//  iNDSEmulationView.h
//  iNDS
//
//  Created by Will Cobb on 1/9/17.
//  Copyright © 2017 DeSmuME Team. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface iNDSEmulationView : UIView

@property GLKView *mainScreen; // NDS main top screen
@property GLKView *touchScreen; // NDS Touch screen
@property (nonatomic, readonly) NSInteger fps;

- (void)updateDisplay;

@end

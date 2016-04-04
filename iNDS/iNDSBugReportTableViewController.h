//
//  iNDSBugReportTableViewController.h
//  inds
//
//  Created by Will Cobb on 3/30/16.
//  Copyright © 2016 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>
@class iNDSGame;

@protocol iNDSBugReportSelectionDelegate <NSObject>

- (void)setGame:(iNDSGame *)game;
- (void)setSaveStatePath:(NSString *)saveStatePath;

@end

@interface iNDSBugReportTableViewController : UITableViewController {
    IBOutlet UITextView *description;
    IBOutlet UILabel    *gameName;
    IBOutlet UILabel    *saveStateName;
    IBOutlet UIButton   *submit;
}

@property (nonatomic) UIImagePickerController *imagePickerController;

@end

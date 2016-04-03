//
//  iNDSAddCheatTableViewController.h
//  iNDS
//
//  Created by Will Cobb on 12/30/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface iNDSAddCheatTableViewController : UITableViewController <UITextViewDelegate>

@property (weak, nonatomic) IBOutlet UITextField    *cheatName;
@property (weak, nonatomic) IBOutlet UITextView     *cheatCode;

@end

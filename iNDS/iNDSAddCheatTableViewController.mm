//
//  iNDSAddCheatTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/30/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSAddCheatTableViewController.h"
#import "SCLAlertView.h"
#import "emu.h"
#import "cheatSystem.h"
@interface iNDSAddCheatTableViewController () {
    BOOL showConfirmation;
}

@end

@implementation iNDSAddCheatTableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    showConfirmation = YES;
}

- (void)textViewDidChange:(UITextView *)textView
{
    showConfirmation = YES;
    NSString *code = self.cheatCode.text;
    code = [[code stringByReplacingOccurrencesOfString:@" " withString:@""] uppercaseString];
    code = [[code stringByReplacingOccurrencesOfString:@"\n" withString:@""] uppercaseString];
    NSMutableString *resultString = [[NSMutableString alloc] init];
    for (NSInteger i = 0; i < code.length; i++) {
        [resultString appendString:[code substringWithRange:NSMakeRange(i, 1)]];
        if ((i + 1)%16 == 0 && i != 0 && i < code.length - 1) {
            [resultString appendString:@"\n"];
        } else if ((i + 1)%8 == 0 && i != 0 && i < code.length - 1) {
            [resultString appendString:@" "];
        }
    }
    
    //Keep cursor coposition
    NSRange cursorPosition = [textView selectedRange];
    textView.text = resultString;
    dispatch_async(dispatch_get_main_queue(), ^{
        textView.selectedRange = NSMakeRange(cursorPosition.location+1, 0);
    });
}


-(BOOL) navigationShouldPopOnBackButton {
    if (showConfirmation) {
        showConfirmation = NO;
        NSLog(@"BYE: %ld %ld", self.cheatCode.text.length + 1, (self.cheatCode.text.length + 1) % 18);
        if ((self.cheatCode.text.length + 1) % 18 == 0) { //A valid cheat code is entered
            dispatch_async(dispatch_get_main_queue(), ^{
                SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
                [alert addButton:@"Don't Save" actionBlock:^{
                    [self.navigationController popViewControllerAnimated:YES];
                }];
                [alert showInfo:self title:@"Wait!" subTitle:@"Are you sure you want to leave without saving this cheat?" closeButtonTitle:@"Stay" duration:0.0];
            });
            return NO;
        }
        return YES;
        
    }
    return YES; 
}

- (IBAction)saveCheat:(id)sender
{
    NSString *code = [self.cheatCode.text stringByReplacingOccurrencesOfString:@"\n" withString:@""];
    NSString *description = [NSString stringWithFormat:@"||%@||", self.cheatName.text];
    cheats->add_AR([code UTF8String], [description UTF8String], NO);
    cheats->save();
    showConfirmation = NO;
    [self.navigationController popViewControllerAnimated:YES];
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

@end

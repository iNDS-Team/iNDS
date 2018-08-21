//
//  iNDSAddCheatTableViewController.m
//  iNDS
//
//  Created by Will Cobb on 12/30/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSAddCheatTableViewController.h"
#import "AppDelegate.h"
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
    if (self.cheatName.text.length < 1) {
        dispatch_async(dispatch_get_main_queue(), ^{
            SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
            [alert showInfo:self title:@"Error!" subTitle:@"Please enter a name for the cheat." closeButtonTitle:@"Okay" duration:0.0];
        });
        return;
    }
    char *code = (char *)[[self.cheatCode.text stringByReplacingOccurrencesOfString:@"\n" withString:@""] UTF8String];
    char *description = (char *)[[NSString stringWithFormat:@"||%@||", self.cheatName.text] UTF8String];
    if (cheats->add_AR(code, description, NO)) {
        cheats->save();
        showConfirmation = NO;
        [self.navigationController popViewControllerAnimated:YES];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            SCLAlertView * alert = [[SCLAlertView alloc] initWithNewWindow];
            [alert showInfo:self title:@"Error!" subTitle:@"Unable to parse cheat. Double check it is entered correctly." closeButtonTitle:@"Okay" duration:0.0];
        });
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    if (section == 0) {
        return [NSString stringWithFormat:@"Game ID: %@", AppDelegate.sharedInstance.currentEmulatorViewController.game.rawTitle];
    }
    return @"";
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

@end

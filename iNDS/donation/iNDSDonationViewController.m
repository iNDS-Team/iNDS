//
//  iNDSDonationViewController.m
//  iNDS
//
//  Created by Developer on 7/10/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSDonationViewController.h"

@interface iNDSDonationViewController ()

@end

@implementation iNDSDonationViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self.navigationController.navigationBar setTintColor:[UIColor colorWithRed:78.0/255.0 green:156.0/255.0 blue:206.0/255.0 alpha:1.0]];

    donateTitle.title = NSLocalizedString(@"DONATE", nil);
    donateLabel.text = NSLocalizedString(@"DONATE_USING_PAYPAL", nil);

}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (NSString *)tableView:(UITableView *)tableView  titleForFooterInSection:(NSInteger)section
{
    NSString *sectionName;
    switch (section)
    {
        case 0:
            sectionName = NSLocalizedString(@"DONATE_USING_PAYPAL_DETAIL", nil);
            break;
        default:
            sectionName = @"";
            break;
    }
    return sectionName;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (indexPath.section == 1 && indexPath.row == 0)
        [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=MCAFUKL3CM8QQ"]];
    
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

@end

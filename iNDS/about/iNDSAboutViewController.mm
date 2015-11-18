//
//  iNDSAboutViewController.m
//  iNDS
//
//  Created by Developer on 7/8/13.
//  Copyright (c) 2014 iNDS. All rights reserved.
//

#import "iNDSAboutViewController.h"
#import "Twitter/Twitter.h"
#import "emu.h"

@interface iNDSAboutViewController ()

@end

@implementation iNDSAboutViewController

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
    
    aboutTitle.title = NSLocalizedString(@"ABOUT_INDS", nil);
    tweetButton.title = NSLocalizedString(@"SHARE", nil);
    versionLabel.text = [NSBundle mainBundle].infoDictionary[@"GitVersionString"];
    desmumeVersion.text = [NSString stringWithCString:EMU_version() encoding:NSASCIIStringEncoding];
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view delegate

- (NSString *)tableView:(UITableView *)tableView  titleForHeaderInSection:(NSInteger)section
{
    NSString *sectionName;
    switch (section)
    {
        case 0:
            sectionName = NSLocalizedString(@"EMULATOR_CORE_CODE", nil);
            break;
        case 1:
            sectionName = NSLocalizedString(@"ABOUT_INDS", nil);
            break;
        default:
            sectionName = @"";
            break;
    }
    return sectionName;
}

- (IBAction)sendTweet:(id)sender {
    //New tweet sheet
    TWTweetComposeViewController *tweetSheet = [[TWTweetComposeViewController alloc] init];
    
    //Preloaded message
    [tweetSheet setInitialText:@"I love playing games on my iOS device with #iNDSEmulator"];
    
    [tweetSheet addURL:[NSURL URLWithString:@"http://nitrogen.reimuhakurei.net/"]];
    
    //Set a blocking handler for the tweet sheet
    tweetSheet.completionHandler = ^(TWTweetComposeViewControllerResult result){
        [self dismissModalViewControllerAnimated:YES];
    };
    
    //Show the tweet sheet
    [self presentModalViewController:tweetSheet animated:YES];
}

@end

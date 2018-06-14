//
//  iNDSInitialViewController.m
//  iNDS
//
//  Created by Will Cobb on 11/27/15.
//  Copyright © 2015 iNDS. All rights reserved.
//

#import "iNDSInitialViewController.h"

@interface iNDSInitialViewController ()

@end

@implementation iNDSInitialViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    NSLog(@"Loaded");
}

-(void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    NSLog(@"Appeared");
    [self performSegueWithIdentifier:@"ToRoot" sender:self];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark - Navigation

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    self.rootView = segue.destinationViewController;
}


@end
